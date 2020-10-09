idf_component_register(INCLUDE_DIRS "." PRIV_REQUIRES idf::mbedtls)

add_definitions(-DCSPOT_USE_MBEDTLS)
add_subdirectory(../../../../cspot bindir)
target_link_libraries(${COMPONENT_LIB} INTERFACE cspot)