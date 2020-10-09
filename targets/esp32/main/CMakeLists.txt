set(COMPONENT_SRCS
    "main.cpp"
    "sinks/I2SAudioSink.cpp"
    "sinks/ac101.c"
)
set(COMPONENT_ADD_INCLUDEDIRS "." "sinks")
register_component(REQUIRES "cspot")
component_compile_options(-std=c++17)