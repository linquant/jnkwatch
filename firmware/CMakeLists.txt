# ESP-IDF project for the ESP32-S3-Touch-AMOLED-1.43 watch.
# Builds the SAME core/ + ui/ sources as the PC emulator; only the HAL differs.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(jnkwatch)
