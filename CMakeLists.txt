cmake_minimum_required(VERSION 2.8.12)

# Configurable options
option(CSPOT_TARGET_CLI "Enable cspotcli target" OFF)
option(CSPOT_TARGET_ESP32 "Enable cspot-esp32 target" OFF)

# Enable cspotcli target
if(CSPOT_TARGET_CLI)
    add_subdirectory(targets/cli)
    project(cspotcli)
endif()

# Enable cspot-esp32 target
if(CSPOT_TARGET_ESP32)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    set(IDF_NO_INCLUDE ON)
    add_subdirectory(targets/esp32)
    project(cspot-esp32)
endif()

