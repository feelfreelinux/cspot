cmake_minimum_required(VERSION 2.8.9)
set (CMAKE_CXX_STANDARD 17)

if(${ESP_PLATFORM})
    add_definitions(-DUSE_MBEDTLS)
    set(EXTRA_REQ_LIBS idf::mbedtls idf::pthread)
    add_definitions(-Wunused-const-variable -Wchar-subscripts -Wunused-label -Wmaybe-uninitialized -Wmisleading-indentation)
else()
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    set(EXTRA_REQ_LIBS OpenSSL::Crypto Threads::Threads)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()


set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra)
find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})

include_directories("include")
include_directories("tremor")
add_definitions(-DPB_ENABLE_MALLOC)
set(PROTOS protocol/proto/authentication.proto protocol/proto/keyexchange.proto protocol/proto/mercury.proto protocol/proto/metadata.proto protocol/proto/playlist.proto protocol/proto/pubsub.proto protocol/proto/radio.proto protocol/proto/spirc.proto)
message(${PROTOS})
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(generate_proto_sources DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

set(SOURCES ${SOURCES} "tremor/mdct.c" "tremor/dsp.c" "tremor/info.c" "tremor/misc.c" "tremor/floor1.c" "tremor/floor0.c" "tremor/vorbisfile.c" "tremor/res012.c" "tremor/mapping0.c" "tremor/codebook.c" "tremor/framing.c" "tremor/bitwise.c" "tremor/floor_lookup.c")
 

add_library(cspot STATIC ${SOURCES} ${PROTO_SRCS})
target_link_libraries(cspot PRIVATE ${EXTRA_REQ_LIBS})

target_include_directories(cspot PUBLIC "include" "tremor" ${EXTRA_REQ_LIBS} ${CMAKE_CURRENT_BINARY_DIR} ${NANOPB_INCLUDE_DIRS})