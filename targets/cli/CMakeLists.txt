project(cspotcli)
cmake_minimum_required(VERSION 3.18) 
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../cspot ${CMAKE_CURRENT_BINARY_DIR}/cspot)

file(GLOB SOURCES "*.cpp")
include_directories(".")

if(UNIX AND NOT APPLE)
    # add_definitions(-DCSPOT_ENABLE_ALSA_SINK)
    # set(THINGS_TO_LINK ${THINGS_TO_LINK} asound)
endif()
add_executable(cspotcli ${SOURCES})
target_link_libraries(cspotcli cspot ${CMAKE_DL_LIBS} ${THINGS_TO_LINK})
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
