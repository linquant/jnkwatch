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
  ui/face_timer.c
  ui/face_compass.c
  ui/face_settings.c
  ui/face_aura.c
  core/watchface_logic.c
  core/dotfont.c
  core/timer.c
  core/compass.c
  core/settings.c
  core/wrist.c
  core/activity.c
  core/notify.c
  core/actions.c)
set(SHARED_INC
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${CMAKE_CURRENT_SOURCE_DIR}/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# --- Headless target: renders a frame to PNG (no display server needed) ---
add_executable(watch_shot shot/main_shot.c ${SHARED_SRC})
target_include_directories(watch_shot PRIVATE ${SHARED_INC})
target_link_libraries(watch_shot lvgl m)

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

add_executable(test_compass
  tests/test_compass.c
  core/compass.c
  third_party/unity/src/unity.c)
target_include_directories(test_compass PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
target_compile_definitions(test_compass PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(test_compass m)
add_test(NAME compass COMMAND test_compass)

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

add_executable(test_actions
  tests/test_actions.c
  core/actions.c
  third_party/unity/src/unity.c)
target_include_directories(test_actions PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src)
add_test(NAME actions COMMAND test_actions)

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
