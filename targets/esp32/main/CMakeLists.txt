set(COMPONENT_SRCS
    "main.cpp"
    "ESPFile.cpp"
    "sinks/AC101AudioSink.cpp"
    "sinks/BufferedAudioSink.cpp"
    "sinks/ES9018AudioSink.cpp"
    "sinks/PCM5102AudioSink.cpp"
    "sinks/InternalAudioSink.cpp"
    "sinks/ac101.c"
)
set(COMPONENT_ADD_INCLUDEDIRS "." "sinks")
register_component(REQUIRES "cspot" "esp_littlefs")
component_compile_options(-std=gnu++17)
