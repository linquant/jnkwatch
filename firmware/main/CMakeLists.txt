# The watch UI/logic live in the repo's core/ + ui/ (shared with the PC build).
# The device HAL is main/device_hal.c; the PC platform/display.c is NOT built.
file(GLOB WATCH_CORE "${CMAKE_CURRENT_LIST_DIR}/../../core/*.c")
file(GLOB WATCH_UI   "${CMAKE_CURRENT_LIST_DIR}/../../ui/*.c")
file(GLOB WATCH_FONT "${CMAKE_CURRENT_LIST_DIR}/../../ui/fonts/*.c")

idf_component_register(
    SRCS "main.c" "device_hal.c" "read_lcd_id.c" "touch_ft3168.c" "rtc_pcf85063.c" "imu_qmi8658.c" ${WATCH_CORE} ${WATCH_UI} ${WATCH_FONT}
    INCLUDE_DIRS "."
        "${CMAKE_CURRENT_LIST_DIR}/../../core"
        "${CMAKE_CURRENT_LIST_DIR}/../../ui"
        "${CMAKE_CURRENT_LIST_DIR}/../../ui/fonts"
        "${CMAKE_CURRENT_LIST_DIR}/../../platform"
    REQUIRES lvgl esp_lvgl_port esp_lcd esp_lcd_sh8601 esp_timer driver esp_psram)

# The generated font uses `#include "lvgl/lvgl.h"` unless this is defined.
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)

# One-shot RTC seeding: build with -DWATCH_SET_RTC_EPOCH=<utc-epoch> to force the
# PCF85063 to that instant on the next boot; omit (or 0) for normal builds.
if(WATCH_SET_RTC_EPOCH)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE WATCH_SET_RTC_EPOCH=${WATCH_SET_RTC_EPOCH})
endif()
