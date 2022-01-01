cmake_minimum_required(VERSION 3.5)

# Main target sources
file(GLOB SOURCES "*.cpp" "*.c")

# Configure the target
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
)

# Build static library, do not build test executables
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTING OFF)

# Import cspot library
add_subdirectory("../../../cspot" ${CMAKE_CURRENT_BINARY_DIR}/cspot)

# Configure the target
target_link_libraries(${COMPONENT_LIB} PUBLIC cspot)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
