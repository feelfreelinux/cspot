cmake_minimum_required(VERSION 2.8.9)
set (CMAKE_CXX_STANDARD 11)
project(cspot)
 

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

include_directories("include")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra)
find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})


set(PROTOS protocol/proto/authentication.proto protocol/proto/keyexchange.proto protocol/proto/mercury.proto protocol/proto/metadata.proto protocol/proto/playlist.proto protocol/proto/pubsub.proto protocol/proto/radio.proto protocol/proto/spirc.proto)
message(${PROTOS})
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(generate_proto_sources DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)


file(GLOB SOURCES "src/*.cpp" "src/*.c")
 
add_executable(cspot ${SOURCES} ${PROTO_SRCS})
target_link_libraries(cspot OpenSSL::Crypto)