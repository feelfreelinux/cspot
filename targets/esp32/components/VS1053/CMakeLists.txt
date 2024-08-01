set(CMAKE_CXX_STANDARD 17)
    idf_component_register(SRCS "src/VS1053.cpp" INCLUDE_DIRS
                                "include"
                                "src" PRIV_REQUIRES fatfs)
