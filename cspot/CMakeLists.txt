project(cspot)

cmake_minimum_required(VERSION 2.8.9)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

if (NOT DEFINED ${USE_EXTERNAL_BELL})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bell ${CMAKE_CURRENT_BINARY_DIR}/bell)
endif()

add_subdirectory("proto")

# Add platform specific sources
# if(${ESP_PLATFORM})
#     file(GLOB ESP_PLATFORM_SOURCES "src/platform/esp/*.cpp" "src/platform/esp/*.c")
#     set(SOURCES ${SOURCES} ${ESP_PLATFORM_SOURCES} )
# endif()

# if(UNIX)
#     file(GLOB UNIX_PLATFORM_SOURCES "src/platform/unix/*.cpp" "src/platform/unix/*.c")
#     set(SOURCES ${SOURCES} ${UNIX_PLATFORM_SOURCES} )
# endif()

# if(APPLE)
#     file(GLOB APPLE_PLATFORM_SOURCES "src/platform/apple/*.cpp" "src/platform/apple/*.c")
#     set(SOURCES ${SOURCES} ${APPLE_PLATFORM_SOURCES} )
# endif()

# if(UNIX AND NOT APPLE)
# #     file(GLOB LINUX_PLATFORM_SOURCES "src/platform/linux/*.cpp" "src/platform/linux/*.c")
# #     set(SOURCES ${SOURCES} ${LINUX_PLATFORM_SOURCES} )
# # endif()


# if(${ESP_PLATFORM})
#     list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoOpenSSL.cpp) # use MBedTLS 
#     idf_build_set_property(COMPILE_DEFINITIONS "-DCSPOT_USE_MBEDTLS" APPEND)
#     set(EXTRA_REQ_LIBS idf::mbedtls idf::pthread idf::mdns)
#     add_definitions(-Wunused-const-variable -Wchar-subscripts -Wunused-label -Wmaybe-uninitialized -Wmisleading-indentation)
# else()
#     list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/CryptoMbedTLS.cpp) # use OpenSSL
#     find_package(OpenSSL REQUIRED)
#     if(OPENSSL_FOUND)
#         set(OPENSSL_USE_STATIC_LIBS TRUE)
#     endif()
#     set(EXTRA_REQ_LIBS OpenSSL::Crypto Threads::Threads)
#     set(THREADS_PREFER_PTHREAD_FLAG ON)
#     find_package(Threads REQUIRED)
# endif()

if(UNIX AND NOT APPLE)
    set(EXTRA_REQ_LIBS ${EXTRA_REQ_LIBS} dns_sd) # add apple bonjur compatibility library for linux
    # TODO: migrate from this to native linux mDNS
endif()

add_library(cspot STATIC ${SOURCES})
target_link_libraries(cspot PRIVATE bell proto ${EXTRA_REQ_LIBS})
target_include_directories(cspot PUBLIC "include" bell ${EXTRA_REQ_LIBS})
