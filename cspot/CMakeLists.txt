project(cspot)

cmake_minimum_required(VERSION 2.8.9)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

# Add platform specific sources

if(${ESP_PLATFORM})
    file(GLOB ESP_PLATFORM_SOURCES "src/platform/esp/*.cpp" "src/platform/esp/*.c")
    set(SOURCES ${SOURCES} ${ESP_PLATFORM_SOURCES} )
endif()

if(UNIX)
    file(GLOB UNIX_PLATFORM_SOURCES "src/platform/unix/*.cpp" "src/platform/unix/*.c")
    set(SOURCES ${SOURCES} ${UNIX_PLATFORM_SOURCES} )
endif()

if(APPLE)
    file(GLOB APPLE_PLATFORM_SOURCES "src/platform/apple/*.cpp" "src/platform/apple/*.c")
    set(SOURCES ${SOURCES} ${APPLE_PLATFORM_SOURCES} )
endif()

if(UNIX AND NOT APPLE)
    file(GLOB LINUX_PLATFORM_SOURCES "src/platform/linux/*.cpp" "src/platform/linux/*.c")
    set(SOURCES ${SOURCES} ${LINUX_PLATFORM_SOURCES} )
endif()



if(${ESP_PLATFORM})
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoOpenSSL.cpp) # use MBedTLS 
    idf_build_set_property(COMPILE_DEFINITIONS "-DCSPOT_USE_MBEDTLS" APPEND)
    set(EXTRA_REQ_LIBS idf::mbedtls idf::pthread idf::mdns)
    add_definitions(-Wunused-const-variable -Wchar-subscripts -Wunused-label -Wmaybe-uninitialized -Wmisleading-indentation)
else()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoOpenSSL.cpp) # use OpenSSL
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if(UNIX AND NOT APPLE)
    set(EXTRA_REQ_LIBS ${EXTRA_REQ_LIBS}) # add apple bonjur compatibility library for linux
    # TODO: migrate from this to native linux mDNS
endif()

include_directories("cJSON")
add_subdirectory(mbedtls EXCLUDE_FROM_ALL)
target_include_directories(mbedtls PUBLIC ./mbedtls/include)
include_directories("include")
include_directories("tremor")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("protos")

message(${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_protos.sh)
execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_protos.sh" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCES ${SOURCES} "tremor/mdct.c" "tremor/dsp.c" "tremor/info.c" "tremor/misc.c" "tremor/floor1.c" "tremor/floor0.c" "tremor/vorbisfile.c" "tremor/res012.c" "tremor/mapping0.c" "tremor/codebook.c" "tremor/framing.c" "tremor/bitwise.c" "tremor/floor_lookup.c" "cJSON/cJSON.c")
set(SOURCES ${SOURCES} "protos/AnyRefImpl.cpp" "protos/ReflectTypeInfo.cpp")

add_library(cspot STATIC ${SOURCES})
target_link_libraries(cspot PRIVATE -static pthread mbedtls ${EXTRA_REQ_LIBS})

target_include_directories(cspot PUBLIC "include" "protos" "tremor" "mbedtls" "cJSON" ${EXTRA_REQ_LIBS} ${CMAKE_CURRENT_BINARY_DIR})
