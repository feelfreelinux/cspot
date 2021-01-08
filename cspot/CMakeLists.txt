project(cspot)

cmake_minimum_required(VERSION 2.8.9)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

if(${ESP_PLATFORM})
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoOpenSSL.cpp)
    idf_build_set_property(COMPILE_DEFINITIONS "-DCSPOT_USE_MBEDTLS" APPEND)
    set(EXTRA_REQ_LIBS idf::mbedtls idf::pthread idf::mdns)
    add_definitions(-Wunused-const-variable -Wchar-subscripts -Wunused-label -Wmaybe-uninitialized -Wmisleading-indentation)
else()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoMbedTLS.cpp)
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    set(EXTRA_REQ_LIBS OpenSSL::Crypto Threads::Threads)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if(UNIX AND NOT APPLE)
    set(EXTRA_REQ_LIBS ${EXTRA_REQ_LIBS} dns_sd)
endif()

include_directories("cJSON")
include_directories("include")
include_directories("tremor")



include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(SOURCES ${SOURCES} "tremor/mdct.c" "tremor/dsp.c" "tremor/info.c" "tremor/misc.c" "tremor/floor1.c" "tremor/floor0.c" "tremor/vorbisfile.c" "tremor/res012.c" "tremor/mapping0.c" "tremor/codebook.c" "tremor/framing.c" "tremor/bitwise.c" "tremor/floor_lookup.c" "cJSON/cJSON.c")


add_library(cspot STATIC ${SOURCES})
target_link_libraries(cspot PRIVATE ${EXTRA_REQ_LIBS})

target_include_directories(cspot PUBLIC "include" "tremor" "cJSON" ${EXTRA_REQ_LIBS} ${CMAKE_CURRENT_BINARY_DIR})