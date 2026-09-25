/* NVS-backed watch configuration store. */
#include "config_store.h"
#include "settings.h"     /* WF_SLEEP_10S */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define CFG_NS "jnkwatch"
static const char *TAG = "cfgstore";

/* Defaults mirror the in-code seeds (wf_settings_init + ACCENT_CYAN). */
static wf_stored_config_t s_cfg = {
    .brightness = 80,
    .sleep_ms   = WF_SLEEP_10S,
    .wrist_wake = true,
    .stay_awake_charging = true,
    .power_saver = false,
    .accent     = 2,   /* ACCENT_CYAN */
};

void config_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t h;
    if (nvs_open(CFG_NS, NVS_READONLY, &h) == ESP_OK) {
        int32_t v;
        if (nvs_get_i32(h, "bright", &v) == ESP_OK) s_cfg.brightness = v;
        if (nvs_get_i32(h, "sleep",  &v) == ESP_OK) s_cfg.sleep_ms   = v;
        if (nvs_get_i32(h, "wrist",  &v) == ESP_OK) s_cfg.wrist_wake = v != 0;
        if (nvs_get_i32(h, "awake",  &v) == ESP_OK) s_cfg.stay_awake_charging = v != 0;
        if (nvs_get_i32(h, "psave",  &v) == ESP_OK) s_cfg.power_saver = v != 0;
        if (nvs_get_i32(h, "accent", &v) == ESP_OK) s_cfg.accent     = v;
        nvs_close(h);
        ESP_LOGI(TAG, "loaded bright=%d sleep=%d wrist=%d awake=%d psave=%d accent=%d",
                 s_cfg.brightness, s_cfg.sleep_ms, s_cfg.wrist_wake,
                 s_cfg.stay_awake_charging, s_cfg.power_saver, s_cfg.accent);
    } else {
        ESP_LOGI(TAG, "no saved config; using defaults");
    }
}

void config_store_get(wf_stored_config_t *out) { *out = s_cfg; }

void config_store_set_settings(int brightness, int sleep_ms, bool wrist_wake,
                               bool stay_awake_charging, bool power_saver,
                               int accent)
{
    s_cfg.brightness = brightness;
    s_cfg.sleep_ms   = sleep_ms;
    s_cfg.wrist_wake = wrist_wake;
    s_cfg.stay_awake_charging = stay_awake_charging;
    s_cfg.power_saver = power_saver;
    s_cfg.accent     = accent;

    nvs_handle_t h;
    if (nvs_open(CFG_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, "bright", brightness);
        nvs_set_i32(h, "sleep",  sleep_ms);
        nvs_set_i32(h, "wrist",  wrist_wake ? 1 : 0);
        nvs_set_i32(h, "awake",  stay_awake_charging ? 1 : 0);
        nvs_set_i32(h, "psave",  power_saver ? 1 : 0);
        nvs_set_i32(h, "accent", accent);
        nvs_commit(h);
        nvs_close(h);
    }
}
