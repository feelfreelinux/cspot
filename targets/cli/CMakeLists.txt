cmake_minimum_required(VERSION 3.0)

project(cspotcli)

# Configurable options
option(USE_ALSA "Enable ALSA" OFF)
option(USE_PORTAUDIO "Enable PortAudio" OFF)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "CMake Build Type")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# CMake options
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
	add_compile_definitions(NOMINMAX _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
	add_definitions(/wd4068 /wd4244 /wd4018 /wd4101 /wd4102 /wd4142)
endif()

# Main target sources
file(GLOB SOURCES "*.cpp")

find_package( portaudio REQUIRED )
include_directories( ${PORTAUDIO_INCLUDE_DIRS} )
set(THINGS_TO_LINK ${THINGS_TO_LINK} ${PORTAUDIO_LIBRARIES})

if(USE_PORTAUDIO)
    add_definitions(-DCSPOT_ENABLE_PORTAUDIO_SINK)
    set(BELL_SINK_PORTAUDIO ON)
endif(USE_PORTAUDIO)
unset(USE_PORTAUDIO CACHE)

if(USE_ALSA)
    add_definitions(-DCSPOT_ENABLE_ALSA_SINK)
    set(BELL_USE_ALSA ON)
endif(USE_ALSA)
unset(USE_ALSA CACHE)

# Import cspot library
add_subdirectory("../../cspot" ${CMAKE_CURRENT_BINARY_DIR}/cspot)

# Configure the target
add_executable(cspotcli ${SOURCES})
target_link_libraries(cspotcli PUBLIC cspot ${EXTRA_LIBS}  ${THINGS_TO_LINK})
target_include_directories(cspotcli PRIVATE "." ${EXTRA_INCLUDES})
