/* Device-side OTA engine: a dedicated task drains BLE writes and streams the
 * image into the inactive app slot via esp_ota_*. All flash work is confined
 * here so the NimBLE host task and the LVGL task never block. The framing /
 * ordering / SHA-256 logic is in core/ota_proto.c (unit-tested on PC). */
#include "ota_update.h"
#include "ota_proto.h"
#include "ble_server.h"
#include "battery.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "ota";

#define OTA_WINDOW          8      /* initial in-flight chunk credits        */
#define OTA_REFILL          4      /* grant credits after this many chunks   */
#define OTA_IDLE_TIMEOUT_MS 10000  /* abort if no frame while active         */
#define OTA_MIN_BATTERY_PCT 40     /* refuse below this unless on charger     */

#define OTA_MSG_CTRL 0
#define OTA_MSG_DATA 1

typedef struct {
    uint8_t  kind;
    uint16_t len;
    uint8_t  buf[WF_OTA_DATA_HDR + WF_OTA_MAX_CHUNK];
} ota_msg_t;

static QueueHandle_t      s_q;
static wf_ota_t           s_proto;
static esp_ota_handle_t   s_handle;
static const esp_partition_t *s_part;
static bool               s_active;
static uint16_t           s_since_credit;
#if CONFIG_PM_ENABLE
static esp_pm_lock_handle_t s_pm_lock;   /* held during a transfer: no light-sleep */
static bool               s_pm_held;
#endif

static void pm_hold(bool on)
{
#if CONFIG_PM_ENABLE
    if (!s_pm_lock) return;
    if (on && !s_pm_held) { esp_pm_lock_acquire(s_pm_lock); s_pm_held = true; }
    else if (!on && s_pm_held) { esp_pm_lock_release(s_pm_lock); s_pm_held = false; }
#else
    (void)on;
#endif
}

static void notify_err(uint8_t code)
{
    uint8_t f[WF_OTA_FRAME_ERROR_LEN];
    ble_ota_notify(f, wf_ota_frame_error(f, code));
}

/* Tear down the current transfer (on error/abort/completion-failure). */
static void cleanup(bool abort_flash)
{
    if (abort_flash && s_handle) esp_ota_abort(s_handle);
    s_handle = 0;
    s_part = NULL;
    s_active = false;
    s_since_credit = 0;
    wf_ota_init(&s_proto);
    ble_ota_set_active(false);
    pm_hold(false);
}

static void handle_start(const uint8_t *buf, int len)
{
    if (s_active) cleanup(true);   /* restart over a stale transfer */

    /* Brownout guard: flashing on a nearly-empty cell risks a bad write. Allow
     * when on external power (charging -> plenty of current, pct unreadable). */
    if (!battery_ext_power() && battery_pct() < OTA_MIN_BATTERY_PCT) {
        ESP_LOGW(TAG, "OTA refused: battery too low and not charging");
        notify_err(WF_OTA_ERR_LOW_BATTERY);
        return;
    }

    s_part = esp_ota_get_next_update_partition(NULL);
    if (!s_part) { notify_err(WF_OTA_ERR_BEGIN); return; }

    uint8_t err = 0;
    int chunk = wf_ota_begin(&s_proto, buf, len, s_part->size, WF_OTA_MAX_CHUNK, &err);
    if (chunk < 0) { notify_err(err); return; }

    esp_err_t e = esp_ota_begin(s_part, OTA_SIZE_UNKNOWN, &s_handle);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(e));
        wf_ota_fail(&s_proto, WF_OTA_ERR_BEGIN);
        notify_err(WF_OTA_ERR_BEGIN);
        return;
    }

    s_active = true;
    s_since_credit = 0;
    pm_hold(true);              /* no light-sleep during the transfer */
    ble_ota_set_active(true);   /* raise connection priority for throughput */

    uint8_t f[WF_OTA_FRAME_READY_LEN];
    ble_ota_notify(f, wf_ota_frame_ready(f, 0, (uint16_t)chunk, OTA_WINDOW));
    ESP_LOGI(TAG, "OTA start -> %s, %u bytes, chunk %d",
             s_part->label, (unsigned)s_proto.total_size, chunk);
}

static void handle_commit(void)
{
    if (!s_active) { notify_err(WF_OTA_ERR_STATE); return; }

    uint8_t err = 0;
    if (wf_ota_commit(&s_proto, &err) < 0) {   /* SHA / short image */
        ESP_LOGE(TAG, "OTA commit rejected: err=%d", err);
        notify_err(err);
        cleanup(true);
        return;
    }
    esp_err_t e = esp_ota_end(s_handle);        /* validates the image header */
    s_handle = 0;
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(e));
        notify_err(WF_OTA_ERR_FLASH);
        cleanup(false);
        return;
    }
    e = esp_ota_set_boot_partition(s_part);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition failed: %s", esp_err_to_name(e));
        notify_err(WF_OTA_ERR_FLASH);
        cleanup(false);
        return;
    }

    uint8_t f[WF_OTA_FRAME_DONE_LEN];
    ble_ota_notify(f, wf_ota_frame_done(f, 0));
    ESP_LOGI(TAG, "OTA done -> booting %s", s_part->label);
    vTaskDelay(pdMS_TO_TICKS(300));   /* let the DONE notify flush */
    esp_restart();
}

static void handle_ctrl(const uint8_t *buf, int len)
{
    if (len < 1) return;
    switch (buf[0]) {
    case WF_OTA_OP_START:  handle_start(buf, len); break;
    case WF_OTA_OP_ABORT:  if (s_active) { ESP_LOGW(TAG, "OTA aborted by phone"); cleanup(true); } break;
    case WF_OTA_OP_COMMIT: handle_commit(); break;
    default: break;
    }
}

static void handle_data(const uint8_t *buf, int len)
{
    if (!s_active) return;   /* stray chunk outside a transfer */

    uint8_t err = 0;
    int payload = wf_ota_feed(&s_proto, buf, len, &err);
    if (payload < 0) {
        ESP_LOGE(TAG, "OTA data rejected: err=%d", err);
        notify_err(err);
        cleanup(true);
        return;
    }
    if (payload > 0) {
        esp_err_t e = esp_ota_write(s_handle, buf + WF_OTA_DATA_HDR, (size_t)payload);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(e));
            notify_err(WF_OTA_ERR_FLASH);
            cleanup(true);
            return;
        }
    }

    /* Replenish credits in batches to keep the pipe full without a notify per
     * chunk; always flush credits once the image is fully received. */
    if (++s_since_credit >= OTA_REFILL || wf_ota_complete(&s_proto)) {
        uint8_t f[WF_OTA_FRAME_CREDIT_LEN];
        ble_ota_notify(f, wf_ota_frame_credit(f, s_proto.next_seq,
                                              s_proto.received, s_since_credit));
        s_since_credit = 0;
    }
}

static void ota_task(void *arg)
{
    (void)arg;
    ota_msg_t m;
    for (;;) {
        TickType_t wait = s_active ? pdMS_TO_TICKS(OTA_IDLE_TIMEOUT_MS) : portMAX_DELAY;
        if (xQueueReceive(s_q, &m, wait) != pdTRUE) {
            if (s_active) {   /* stalled mid-transfer */
                ESP_LOGW(TAG, "OTA timeout");
                notify_err(WF_OTA_ERR_TIMEOUT);
                cleanup(true);
            }
            continue;
        }
        if (m.kind == OTA_MSG_CTRL) handle_ctrl(m.buf, m.len);
        else                        handle_data(m.buf, m.len);
    }
}

void ota_update_init(void)
{
    wf_ota_init(&s_proto);
#if CONFIG_PM_ENABLE
    esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "ota", &s_pm_lock);
#endif
    s_q = xQueueCreate(OTA_WINDOW + 2, sizeof(ota_msg_t));
    if (!s_q) { ESP_LOGE(TAG, "queue alloc failed"); return; }
    /* Priority 2 = below the LVGL task (3) so the UI stays responsive, above
     * idle so flash writes still progress. */
    xTaskCreate(ota_task, "ota", 4096, NULL, 2, NULL);
}

static void enqueue(uint8_t kind, const uint8_t *buf, int len)
{
    if (!s_q || len < 0 || len > (int)sizeof(((ota_msg_t *)0)->buf)) return;
    ota_msg_t m;
    m.kind = kind;
    m.len = (uint16_t)len;
    for (int i = 0; i < len; i++) m.buf[i] = buf[i];
    xQueueSend(s_q, &m, 0);   /* drop if full; credit flow keeps this from happening */
}

void ota_update_on_ctrl(const uint8_t *buf, int len) { enqueue(OTA_MSG_CTRL, buf, len); }
void ota_update_on_data(const uint8_t *buf, int len) { enqueue(OTA_MSG_DATA, buf, len); }

void ota_update_abort(void)
{
    uint8_t op = WF_OTA_OP_ABORT;
    enqueue(OTA_MSG_CTRL, &op, 1);
}
