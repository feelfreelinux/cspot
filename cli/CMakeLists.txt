project(cspot)
cmake_minimum_required(VERSION 3.18) 
set (CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cspot ${CMAKE_CURRENT_BINARY_DIR}/cspot)

file(GLOB SOURCES "*.cpp")
 
add_executable(cspot ${SOURCES})
target_link_libraries(cspot CSpotLib ${CMAKE_DL_LIBS})