set(COMPONENT_SRCS
    "main.cpp"
    "I2SAudioSink.cpp"
    "ac101.c"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component(REQUIRES "cspot")
component_compile_options(-std=c++17)