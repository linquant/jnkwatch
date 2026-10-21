cmake_minimum_required(VERSION 3.16)
project(watch C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Make our lv_conf.h discoverable via #include "lv_conf.h".
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(LV_CONF_INCLUDE_SIMPLE ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)
add_subdirectory(lvgl)

# Portable code shared by every target.
set(SHARED_SRC
  platform/display.c
  ui/ui.c
  ui/theme.c
  ui/fonts/wf_digits_100.c
  ui/face_clock.c
  ui/face_notif.c
  ui/face_actions.c
  ui/face_settings.c
  ui/face_debug.c
  ui/face_aura.c
  ui/face_apps.c
  ui/face_level.c
  ui/face_steps.c
  ui/face_probes.c
  core/watchface_logic.c
  core/dotfont.c
  core/timer.c
  core/settings.c
  core/wrist.c
  core/activity.c
  core/notify.c
  core/actions.c
  core/pixelstats.c
  core/screentime.c
  core/weather.c
  core/level.c
  core/pedometer.c
  core/steps.c
  core/probes.c
  core/sha256.c
  core/ota_proto.c)
set(SHARED_INC
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${CMAKE_CURRENT_SOURCE_DIR}/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# --- Headless target: renders a frame to PNG (no display server needed) ---
add_executable(watch_shot shot/main_shot.c ${SHARED_SRC})
target_include_directories(watch_shot PRIVATE ${SHARED_INC})
target_link_libraries(watch_shot lvgl m)

# --- Headless target: per-face black/lit pixel ratio (AMOLED budget) ---
add_executable(watch_pixels shot/main_pixels.c ${SHARED_SRC})
target_include_directories(watch_pixels PRIVATE ${SHARED_INC})
target_link_libraries(watch_pixels lvgl m)

# --- Unit tests (pure logic, no LVGL/SDL needed) ---
enable_testing()
add_executable(test_watchface_logic
  tests/test_watchface_logic.c
  core/watchface_logic.c
  third_party/unity/src/unity.c)
target_include_directories(test_watchface_logic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_watchface_logic PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_watchface_logic m)
add_test(NAME watchface_logic COMMAND test_watchface_logic)

add_executable(test_pedometer
  tests/test_pedometer.c
  core/pedometer.c
  third_party/unity/src/unity.c)
target_include_directories(test_pedometer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_pedometer PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_pedometer m)
add_test(NAME pedometer COMMAND test_pedometer)

add_executable(test_steps
  tests/test_steps.c
  core/steps.c
  core/pedometer.c
  third_party/unity/src/unity.c)
target_include_directories(test_steps PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_steps PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_steps m)
add_test(NAME steps COMMAND test_steps)

add_executable(test_dotfont
  tests/test_dotfont.c
  core/dotfont.c
  third_party/unity/src/unity.c)
target_include_directories(test_dotfont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME dotfont COMMAND test_dotfont)

add_executable(test_timer
  tests/test_timer.c
  core/timer.c
  third_party/unity/src/unity.c)
target_include_directories(test_timer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_timer PRIVATE UNITY_INCLUDE_DOUBLE)
add_test(NAME timer COMMAND test_timer)

add_executable(test_settings
  tests/test_settings.c
  core/settings.c
  third_party/unity/src/unity.c)
target_include_directories(test_settings PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME settings COMMAND test_settings)

add_executable(test_wrist
  tests/test_wrist.c
  core/wrist.c
  third_party/unity/src/unity.c)
target_include_directories(test_wrist PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_wrist PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_wrist m)
add_test(NAME wrist COMMAND test_wrist)

add_executable(test_level
  tests/test_level.c
  core/level.c
  third_party/unity/src/unity.c)
target_include_directories(test_level PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_level PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_level m)
add_test(NAME level COMMAND test_level)

add_executable(test_activity
  tests/test_activity.c
  core/activity.c
  third_party/unity/src/unity.c)
target_include_directories(test_activity PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_activity PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_activity m)
add_test(NAME activity COMMAND test_activity)

add_executable(test_notify
  tests/test_notify.c
  core/notify.c
  third_party/unity/src/unity.c)
target_include_directories(test_notify PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME notify COMMAND test_notify)

add_executable(test_weather
  tests/test_weather.c
  core/weather.c
  third_party/unity/src/unity.c)
target_include_directories(test_weather PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME weather COMMAND test_weather)

add_executable(test_probes
  tests/test_probes.c
  core/probes.c
  third_party/unity/src/unity.c)
target_include_directories(test_probes PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME probes COMMAND test_probes)

add_executable(test_actions
  tests/test_actions.c
  core/actions.c
  third_party/unity/src/unity.c)
target_include_directories(test_actions PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME actions COMMAND test_actions)

add_executable(test_pixelstats
  tests/test_pixelstats.c
  core/pixelstats.c
  third_party/unity/src/unity.c)
target_include_directories(test_pixelstats PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_pixelstats PRIVATE UNITY_INCLUDE_DOUBLE)
add_test(NAME pixelstats COMMAND test_pixelstats)

add_executable(test_screentime
  tests/test_screentime.c
  core/screentime.c
  third_party/unity/src/unity.c)
target_include_directories(test_screentime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME screentime COMMAND test_screentime)

add_executable(test_ota_proto
  tests/test_ota_proto.c
  core/ota_proto.c
  core/sha256.c
  third_party/unity/src/unity.c)
target_include_directories(test_ota_proto PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME ota_proto COMMAND test_ota_proto)

# AMOLED lit-pixel budget guard (renders each face; fails if over budget).
add_test(NAME amoled_budget COMMAND watch_pixels --check)

# --- Interactive target: SDL2 window (only built if SDL2 is installed) ---
find_package(SDL2 QUIET)
if(SDL2_FOUND)
  add_executable(watch_sim sim/main_sim.c ${SHARED_SRC})
  target_include_directories(watch_sim PRIVATE ${SHARED_INC} ${SDL2_INCLUDE_DIRS})
  if(TARGET SDL2::SDL2)
    target_link_libraries(watch_sim lvgl m SDL2::SDL2)
  else()
    target_link_libraries(watch_sim lvgl m ${SDL2_LIBRARIES})
  endif()
  message(STATUS "watch_sim: ENABLED (SDL2 found)")
else()
  message(STATUS "watch_sim: SKIPPED -- run 'sudo apt install libsdl2-dev' to enable")
endif()
