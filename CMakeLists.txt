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


file(GLOB PROTOS authentication.proto keyexchange.proto mercury.proto metadata.proto playlist.proto pubsub.proto radio.proto spirc.proto )
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#add_custom_target(generate_proto_sources DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)


file(GLOB SOURCES "src/*.cpp" "src/*.c")
 
add_executable(cspot ${SOURCES} ${PROTO_SRCS})
target_link_libraries(cspot OpenSSL::Crypto)