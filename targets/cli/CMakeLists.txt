cmake_minimum_required(VERSION 3.0)

project(cspotcli)

# Configurable options
option(USE_ALSA "Enable ALSA" OFF)
option(USE_PORTAUDIO "Enable PortAudio" OFF)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "CMake Build Type")

# CMake options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Main target sources
file(GLOB SOURCES "*.cpp")

if(USE_PORTAUDIO)
    add_definitions(-DCSPOT_ENABLE_PORTAUDIO_SINK)
    find_package(portaudio REQUIRED)
    list(APPEND EXTRA_INCLUDES ${PORTAUDIO_INCLUDE_DIRS})
    list(APPEND EXTRA_LIBS ${PORTAUDIO_LIBRARIES})
endif(USE_PORTAUDIO)
unset(USE_PORTAUDIO CACHE)

if(USE_ALSA)
    add_definitions(-DCSPOT_ENABLE_ALSA_SINK)
    find_package(ALSA REQUIRED)
    list(APPEND EXTRA_INCLUDES ${ALSA_INCLUDE_DIRS})
    list(APPEND EXTRA_LIBS ${ALSA_LIBRARIES})
endif(USE_ALSA)
unset(USE_ALSA CACHE)

# Import cspot library
add_subdirectory("../../cspot" ${CMAKE_CURRENT_BINARY_DIR}/cspot)

# Configure the target
add_executable(cspotcli ${SOURCES})
target_link_libraries(cspotcli PUBLIC cspot ${EXTRA_LIBS})
target_include_directories(cspotcli PRIVATE "." ${EXTRA_INCLUDES})