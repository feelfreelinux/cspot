set(CMAKE_CXX_STANDARD 17)
    idf_component_register( SRCS    "src/VS1053.cpp"
                                    "../../../../cspot/bell/main/utilities/BellLogger.cpp"
                                    "../../../../cspot/bell/main/platform/esp/WrappedSemaphore.cpp"
                            INCLUDE_DIRS
                                    "include"
                                    "../../../../cspot/bell/main/utilities/include"
                                    "../../../../cspot/bell/main/platform/"
                            PRIV_REQUIRES fatfs)
