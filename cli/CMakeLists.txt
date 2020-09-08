project(cspotcli)
cmake_minimum_required(VERSION 3.18) 
set (CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cspot ${CMAKE_CURRENT_BINARY_DIR}/cspot)

file(GLOB SOURCES "*.cpp")

add_definitions(-DUSE_MBEDTLS)

add_executable(cspotcli ${SOURCES})
target_link_libraries(cspotcli CSpotLib ${CMAKE_DL_LIBS})