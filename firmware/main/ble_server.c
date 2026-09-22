/* NimBLE GATT server for the companion app. Exposes the "JNKWatch Config"
 * service; Phase 1 implements the Settings characteristic (read/write/notify).
 * See docs/ble-protocol.md for the wire contract. Runs on its own host task —
 * writes are handed to the LVGL side via device_config_enqueue(). */
#include "ble_server.h"
#include "device_hal.h"     /* device_config_enqueue, wf_config_msg_t */
#include "config_store.h"

#include <string.h>
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

static const ble_uuid128_t svc_uuid      = JNK_UUID128(0x00);
static const ble_uuid128_t settings_uuid = JNK_UUID128(0x01);

static uint8_t  s_addr_type;
static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_settings_val_handle;

static int  gap_event(struct ble_gap_event *event, void *arg);
static void advertise(void);

/* Pack the current config into the 4-byte Settings payload. */
static void settings_pack(uint8_t buf[4])
{
    wf_stored_config_t c;
    config_store_get(&c);
    buf[0] = (uint8_t)c.brightness;
    buf[1] = (uint8_t)(c.sleep_ms >= 7500 ? 1 : 0);
    buf[2] = c.wrist_wake ? 1 : 0;
    buf[3] = (uint8_t)c.accent;
}

static int settings_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR: {
        uint8_t buf[4];
        settings_pack(buf);
        return os_mbuf_append(ctxt->om, buf, sizeof(buf)) == 0
                   ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    case BLE_GATT_ACCESS_OP_WRITE_CHR: {
        uint8_t buf[4];
        uint16_t len = 0;
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len) != 0 || len < 4)
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        int brightness = buf[0] < 10 ? 10 : (buf[0] > 100 ? 100 : buf[0]);
        int sleep_ms   = buf[1] ? 10000 : 5000;
        bool wrist     = buf[2] != 0;
        int accent     = buf[3] % 6;
        config_store_set_settings(brightness, sleep_ms, wrist, accent);
        wf_config_msg_t m = {
            .kind = WF_CFG_SETTINGS, .brightness = brightness,
            .sleep_ms = sleep_ms, .wrist_wake = wrist, .accent = accent,
        };
        device_config_enqueue(&m);
        ESP_LOGI(TAG, "settings write: b=%d s=%d w=%d a=%d",
                 brightness, sleep_ms, wrist, accent);
        return 0;
    }
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
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
                         BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ_ENC |
                         BLE_GATT_CHR_F_WRITE_ENC,
                .val_handle = &s_settings_val_handle,
            },
            { 0 },
        },
    },
    { 0 },
};

void ble_settings_notify(int brightness, int sleep_ms, bool wrist_wake, int accent)
{
    if (s_conn == BLE_HS_CONN_HANDLE_NONE) return;
    uint8_t buf[4] = {
        (uint8_t)brightness, (uint8_t)(sleep_ms >= 7500 ? 1 : 0),
        wrist_wake ? 1 : 0, (uint8_t)accent,
    };
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (om) ble_gatts_notify_custom(s_conn, s_settings_val_handle, om);
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

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn = event->connect.conn_handle;
            ESP_LOGI(TAG, "connected");
        } else {
            advertise();
        }
        return 0;
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
