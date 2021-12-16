cmake_minimum_required(VERSION 2.8.12)

project(cspot)

# Configurable options
set(CSPOT_EXTERNAL_BELL "" CACHE STRING "External bell library target name, optional")

# CMake options
set(CMAKE_CXX_STANDARD 17)

# Main library sources
file(GLOB SOURCES "src/*.cpp" "src/*.c")

# Use externally specified bell library or the submodule
if(CSPOT_EXTERNAL_BELL)
    list(APPEND EXTRA_LIBS ${CSPOT_EXTERNAL_BELL})
else()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bell)
    list(APPEND EXTRA_LIBS bell)
endif()

# Use protobuf library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/proto)
list(APPEND EXTRA_LIBS proto)

# Add Apple Bonjour compatibility library for Linux
if(UNIX AND NOT APPLE)
    list(APPEND EXTRA_LIBS dns_sd)
    # TODO: migrate from this to native linux mDNS
endif()

add_library(cspot STATIC ${SOURCES})
# PUBLIC to propagate includes from bell to cspot dependents
target_link_libraries(cspot PUBLIC ${EXTRA_LIBS})
target_include_directories(cspot PUBLIC "include" ${CMAKE_CURRENT_BINARY_DIR})
