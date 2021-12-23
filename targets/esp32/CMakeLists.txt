# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Don't override EXTRA_COMPONENT_DIRS as platformio uses it.  Instead we append
# see https://github.com/platformio/platform-espressif32/issues/341
list(APPEND EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

if(NOT IDF_NO_INCLUDE)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
endif()
project(cspot-esp32)