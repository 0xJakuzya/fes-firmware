cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32")
set(EXTRA_COMPONENT_DIRS "firmware")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)
