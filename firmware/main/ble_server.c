/* NimBLE GATT server for the companion app. Exposes the "JNKWatch Config"
 * service; Phase 1 implements the Settings characteristic (read/write/notify).
 * See docs/ble-protocol.md for the wire contract. Runs on its own host task —
 * writes are handed to the LVGL side via device_config_enqueue(). */
#include "ble_server.h"
#include "device_hal.h"     /* device_config_enqueue, device_notif_enqueue */
#include "config_store.h"
#include "rtc_pcf85063.h"
#include "notify.h"         /* wf_notif_t + field lengths */
#include "wrist.h"          /* WF_WRIST_SENS_COUNT / WF_WRIST_SENS_DEFAULT */

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

void ble_store_config_init(void);   /* from the store/config NimBLE module */

static const char *TAG = "ble";

/* UUID family 6a4e01NN-9b7a-4f2c-8d61-2a1b3c4d5e6f (bytes little-endian). */
#define JNK_UUID128(nn) BLE_UUID128_INIT( \
    0x6f, 0x5e, 0x4d, 0x3c, 0x1b, 0x2a, 0x61, 0x8d, \
    0x2c, 0x4f, 0x7a, 0x9b, (nn), 0x01, 0x4e, 0x6a)

#define FW_MAJOR 1
#define FW_MINOR 0

static const ble_uuid128_t svc_uuid       = JNK_UUID128(0x00);
static const ble_uuid128_t settings_uuid  = JNK_UUID128(0x01);
static const ble_uuid128_t time_uuid      = JNK_UUID128(0x02);
static const ble_uuid128_t notifpush_uuid = JNK_UUID128(0x03);
static const ble_uuid128_t notifclr_uuid  = JNK_UUID128(0x04);
static const ble_uuid128_t actioncfg_uuid = JNK_UUID128(0x05);
static const ble_uuid128_t actionevt_uuid = JNK_UUID128(0x06);
static const ble_uuid128_t status_uuid    = JNK_UUID128(0x07);
static const ble_uuid128_t weather_uuid   = JNK_UUID128(0x08);

static uint8_t  s_addr_type;
static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_settings_val_handle;
static uint16_t s_action_val_handle;

static int  gap_event(struct ble_gap_event *event, void *arg);
static void advertise(void);

/* Pack the current config into the 7-byte Settings payload.
 * byte4 (stay_awake_charging), byte5 (power_saver) and byte6 (wrist_sens) were
 * appended after the app's original 4 bytes; shorter old clients still work
 * (see settings_access). */
static void settings_pack(uint8_t buf[7])
{
    wf_stored_config_t c;
    config_store_get(&c);
    buf[0] = (uint8_t)c.brightness;
    buf[1] = (uint8_t)(c.sleep_ms >= 7500 ? 1 : 0);
    buf[2] = c.wrist_wake ? 1 : 0;
    buf[3] = (uint8_t)c.accent;
    buf[4] = c.stay_awake_charging ? 1 : 0;
    buf[5] = c.power_saver ? 1 : 0;
    buf[6] = (uint8_t)c.wrist_sens;
}

static int settings_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR: {
        uint8_t buf[6];
        settings_pack(buf);
        return os_mbuf_append(ctxt->om, buf, sizeof(buf)) == 0
                   ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    case BLE_GATT_ACCESS_OP_WRITE_CHR: {
        uint8_t buf[6] = {0};
        uint16_t len = 0;
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len) != 0 || len < 4)
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        int brightness = buf[0] < 10 ? 10 : (buf[0] > 100 ? 100 : buf[0]);
        int sleep_ms   = buf[1] ? 10000 : 5000;
        bool wrist     = buf[2] != 0;
        int accent     = buf[3] % 6;
        /* byte4/5/6 optional: absent (old client) => keep screen on when
         * charging, power-saver off, default (Moyenne) wrist sensitivity. */
        bool stay_awake  = (len >= 5) ? (buf[4] != 0) : true;
        bool power_saver = (len >= 6) ? (buf[5] != 0) : false;
        int wrist_sens   = (len >= 7) ? (buf[6] % WF_WRIST_SENS_COUNT)
                                      : WF_WRIST_SENS_DEFAULT;
        config_store_set_settings(brightness, sleep_ms, wrist, wrist_sens,
                                  stay_awake, power_saver, accent);
        wf_config_msg_t m = {
            .kind = WF_CFG_SETTINGS, .brightness = brightness,
            .sleep_ms = sleep_ms, .wrist_wake = wrist, .wrist_sens = wrist_sens,
            .stay_awake_charging = stay_awake, .power_saver = power_saver,
            .accent = accent,
        };
        device_config_enqueue(&m);
        ESP_LOGI(TAG, "settings write: b=%d s=%d w=%d wsens=%d awake=%d psave=%d a=%d",
                 brightness, sleep_ms, wrist, wrist_sens, stay_awake, power_saver, accent);
        return 0;
    }
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

/* Time (W): u64 LE epoch seconds UTC → RTC + system clock. */
static int time_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint8_t b[8];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, b, sizeof(b), &len) != 0 || len < 8)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    uint64_t epoch = 0;
    for (int i = 0; i < 8; i++) epoch |= (uint64_t)b[i] << (8 * i);
    struct tm g;
    time_t t = (time_t)epoch;
    gmtime_r(&t, &g);
    rtc_pcf85063_set(&g);                 /* RTC holds UTC */
    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);
    ESP_LOGI(TAG, "time set: epoch=%llu", (unsigned long long)epoch);
    return 0;
}

/* NotifPush (W): variable UTF-8, NUL-separated "app\0title\0body". The watch
 * timestamps it with its own clock and truncates each field (wf_notif_push). */
static const char *nul_field(const char *buf, int buflen, int *pos)
{
    if (*pos >= buflen) return "";
    const char *s = buf + *pos;
    int l = (int)strnlen(s, buflen - *pos);
    *pos += l + 1;
    return s;   /* buffer is NUL-terminated by the caller */
}

static int notifpush_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    char b[256];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, b, sizeof(b) - 1, &len) != 0 || len == 0)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    b[len] = '\0';

    int pos = 0;
    const char *app   = nul_field(b, len, &pos);
    const char *title = nul_field(b, len, &pos);
    const char *body  = nul_field(b, len, &pos);

    wf_notif_t n = {0};
    snprintf(n.app,   sizeof(n.app),   "%s", app);
    snprintf(n.title, sizeof(n.title), "%s", title);
    snprintf(n.body,  sizeof(n.body),  "%s", body);
    n.ts = (long)time(NULL);
    device_notif_enqueue(&n);
    ESP_LOGI(TAG, "notif push: app='%s' title='%s' body='%s'", n.app, n.title, n.body);
    return 0;
}

/* NotifClear (W): any value clears the store. */
static int notifclr_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    wf_config_msg_t m = { .kind = WF_CFG_NOTIF_CLEAR };
    device_config_enqueue(&m);
    return 0;
}

/* ActionCfg (W): op=0 clear; op=1 set {index,id,icon,enabled,label[12]}. */
static int actioncfg_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint8_t b[20];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, b, sizeof(b), &len) != 0 || len < 1)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    if (b[0] == 0x00) {
        wf_config_msg_t m = { .kind = WF_CFG_ACTION_CLEAR };
        device_config_enqueue(&m);
        return 0;
    }
    if (b[0] == 0x01 && len >= 20) {
        wf_config_msg_t m = { .kind = WF_CFG_ACTION_SET };
        m.index = b[1];
        memcpy(&m.action_id, b + 2, 4);
        m.icon = b[6];
        m.enabled = b[7] != 0;
        memcpy(m.label, b + 8, 12);
        m.label[12] = '\0';
        device_config_enqueue(&m);
        return 0;
    }
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
}

/* ActionEvent is notify-only; a read/write is never permitted. */
static int evt_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)ctxt; (void)arg;
    return BLE_ATT_ERR_UNLIKELY;
}

/* Status (R/notify): battery u8 + fw major/minor. */
static int status_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint8_t b[3] = { (uint8_t)watch_battery_pct(), FW_MAJOR, FW_MINOR };
    return os_mbuf_append(ctxt->om, b, sizeof(b)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* Characteristic User Description (0x2901): a human-readable name so tools like
 * nRF Connect show "Time" etc. instead of a bare UUID. The name is passed as arg. */
static int dsc_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a;
    const char *name = (const char *)arg;
    return os_mbuf_append(ctxt->om, name, strlen(name)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

#define CUD(name) (struct ble_gatt_dsc_def[]){ \
        { .uuid = BLE_UUID16_DECLARE(0x2901), .att_flags = BLE_ATT_F_READ, \
          .access_cb = dsc_access, .arg = (void *)(name) }, \
        { 0 } }

/* Weather (W): [temp int16 LE][cond u8][location UTF-8 NUL]. */
static int weather_access(uint16_t c, uint16_t a, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)c; (void)a; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint8_t b[64];
    uint16_t len = 0;
    if (ble_hs_mbuf_to_flat(ctxt->om, b, sizeof(b) - 1, &len) != 0 || len < 3)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    b[len] = '\0';
    wf_config_msg_t m = { .kind = WF_CFG_WEATHER };
    m.temp_c = (int16_t)(b[0] | (b[1] << 8));   /* LE, signed */
    m.cond = b[2];
    size_t loclen = strlen((const char *)&b[3]);
    if (loclen >= sizeof(m.location)) loclen = sizeof(m.location) - 1;
    memcpy(m.location, &b[3], loclen);
    m.location[loclen] = '\0';
    device_config_enqueue(&m);
    ESP_LOGI(TAG, "weather: %dC cond=%d loc='%s'", m.temp_c, m.cond, m.location);
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &settings_uuid.u,
                .access_cb = settings_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                         BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_settings_val_handle,
                .descriptors = CUD("Settings"),
            },
            {
                .uuid = &time_uuid.u,
                .access_cb = time_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = CUD("Time (epoch u64 LE)"),
            },
            {
                .uuid = &notifpush_uuid.u,
                .access_cb = notifpush_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = CUD("Notification push"),
            },
            {
                .uuid = &notifclr_uuid.u,
                .access_cb = notifclr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = CUD("Notification clear"),
            },
            {
                .uuid = &actioncfg_uuid.u,
                .access_cb = actioncfg_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = CUD("Action config"),
            },
            {
                .uuid = &actionevt_uuid.u,
                .access_cb = evt_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_action_val_handle,
                .descriptors = CUD("Action event"),
            },
            {
                .uuid = &status_uuid.u,
                .access_cb = status_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = CUD("Status"),
            },
            {
                .uuid = &weather_uuid.u,
                .access_cb = weather_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = CUD("Weather"),
            },
            { 0 },
        },
    },
    { 0 },
};

void ble_settings_notify(int brightness, int sleep_ms, bool wrist_wake,
                         int wrist_sens, bool stay_awake_charging,
                         bool power_saver, int accent)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
    uint8_t buf[7] = {
        (uint8_t)brightness, (uint8_t)(sleep_ms >= 7500 ? 1 : 0),
        wrist_wake ? 1 : 0, (uint8_t)accent, stay_awake_charging ? 1 : 0,
        power_saver ? 1 : 0, (uint8_t)wrist_sens,
    };
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (om) ble_gatts_notify_custom(s_conn, s_settings_val_handle, om);
}

void ble_action_notify(unsigned action_id)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
    uint8_t buf[4];
    for (int i = 0; i < 4; i++) buf[i] = (uint8_t)(action_id >> (8 * i));
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (om) ble_gatts_notify_custom(s_conn, s_action_val_handle, om);
}

static void advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    struct ble_hs_adv_fields rsp = {0};
    struct ble_gap_adv_params advp = {0};
    const char *name = ble_svc_gap_device_name();

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    /* Service UUID in the scan response (won't fit alongside the name in adv). */
    rsp.uuids128 = (ble_uuid128_t *)&svc_uuid;
    rsp.num_uuids128 = 1;
    rsp.uuids128_is_complete = 1;
    ble_gap_adv_rsp_set_fields(&rsp);

    advp.conn_mode = BLE_GAP_CONN_MODE_UND;
    advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &advp, gap_event, NULL);
}

/* Ask the phone for power-friendly connection parameters once connected. With
 * slave latency the watch may skip connection events while idle (radio + CPU
 * stay asleep in light-sleep), yet a notification is still delivered within
 * ~interval*(latency+1). Units: interval = 1.25 ms, timeout = 10 ms.
 *   45-70 ms interval, latency 20  -> idle wake ~1.5 s
 *   supervision 6 s  ( > 2*70ms*(20+1) = 2.94 s, and < the 32 s BLE ceiling ). */
static void request_low_power_conn_params(uint16_t conn_handle)
{
    struct ble_gap_upd_params p = {
        .itvl_min = 36,             /* 45 ms  */
        .itvl_max = 56,             /* 70 ms  */
        .latency = 20,              /* skip up to 20 events when idle */
        .supervision_timeout = 600, /* 6 s    */
        .min_ce_len = 0,
        .max_ce_len = 0,
    };
    int rc = ble_gap_update_params(conn_handle, &p);
    ESP_LOGI(TAG, "conn param request (45-70ms, lat 20, sup 6s): rc=%d", rc);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn = event->connect.conn_handle;
            ESP_LOGI(TAG, "connected");
            request_low_power_conn_params(s_conn);
        } else {
            advertise();
        }
        return 0;
    case BLE_GAP_EVENT_CONN_UPDATE: {
        /* Log the parameters actually in force after negotiation. */
        struct ble_gap_conn_desc d;
        if (ble_gap_conn_find(event->conn_update.conn_handle, &d) == 0)
            ESP_LOGI(TAG, "conn params now: itvl=%d (%.2f ms) latency=%d timeout=%d (%d ms)",
                     d.conn_itvl, d.conn_itvl * 1.25, d.conn_latency,
                     d.supervision_timeout, d.supervision_timeout * 10);
        return 0;
    }
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected (reason %d)", event->disconnect.reason);
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        advertise();
        return 0;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        advertise();
        return 0;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* Bonded peer re-pairing: drop the stale bond and let it proceed. */
        {
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0)
                ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    default:
        return 0;
    }
}

static void on_sync(void)
{
    ble_hs_id_infer_auto(0, &s_addr_type);
    advertise();
    ESP_LOGI(TAG, "advertising as JNKWatch");
}

static void on_reset(int reason) { ESP_LOGW(TAG, "nimble reset; reason=%d", reason); }

static void host_task(void *param)
{
    (void)param;
    nimble_port_run();               /* returns only at nimble_port_stop() */
    nimble_port_freertos_deinit();
}

void ble_server_start(void)
{
    if (nimble_port_init() != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed");
        return;
    }
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    /* Bonding + secure connections; store keys in NVS. */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_svc_gap_device_name_set("JNKWatch");

    ble_store_config_init();
    nimble_port_freertos_init(host_task);
}
