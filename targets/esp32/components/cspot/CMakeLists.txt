idf_component_register(INCLUDE_DIRS "." PRIV_REQUIRES idf::mbedtls)

add_definitions(-DCSPOT_USE_MBEDTLS)
set(BELL_DISABLE_CODECS 1)
set(BELL_MINIMAL 1)
# set(BELL_DISABLE_TREMOR 1)
# set(BELL_DISABLE_CJSON 1)
add_subdirectory(../../../../cspot bindir)
target_link_libraries(${COMPONENT_LIB} INTERFACE cspot)
