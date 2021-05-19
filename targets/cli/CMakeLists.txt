project(cspotcli)
cmake_minimum_required(VERSION 3.0) 
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(USE_ALSA "Enable ALSA" OFF)
option(USE_PORTAUDIO "Enable PortAudio" OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../cspot ${CMAKE_CURRENT_BINARY_DIR}/cspot)

file(GLOB SOURCES "*.cpp")
include_directories(".")

if(USE_PORTAUDIO)
    add_definitions(-DCSPOT_ENABLE_PORTAUDIO_SINK)
    find_package( portaudio REQUIRED )
    include_directories( ${PORTAUDIO_INCLUDE_DIRS} )
    set(THINGS_TO_LINK ${THINGS_TO_LINK} ${PORTAUDIO_LIBRARIES})
endif(USE_PORTAUDIO)
unset(USE_PORTAUDIO CACHE)

if(USE_ALSA)
    add_definitions(-DCSPOT_ENABLE_ALSA_SINK)
    find_package(ALSA REQUIRED)
    include_directories(${ALSA_INCLUDE_DIRS})
    set(THINGS_TO_LINK ${THINGS_TO_LINK} ${ALSA_LIBRARIES})
endif(USE_ALSA)
unset(USE_ALSA CACHE)


add_executable(cspotcli ${SOURCES})
target_link_libraries(cspotcli cspot ${CMAKE_DL_LIBS} ${THINGS_TO_LINK} )
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
