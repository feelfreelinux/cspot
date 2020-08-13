cmake_minimum_required(VERSION 2.8.9)
project(cspot)
 

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

include_directories(include)

file(GLOB SOURCES "src/*.cpp" "src/*.c")
 
add_executable(cspot ${SOURCES})
target_link_libraries(cspot OpenSSL::Crypto)