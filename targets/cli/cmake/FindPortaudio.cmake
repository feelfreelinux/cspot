#  PORTAUDIO_FOUND - system has libportaudio
#  PORTAUDIO_INCLUDE_DIRS - the libportaudio include directory
#  PORTAUDIO_LIBRARIES - Link these to use libportaudio

if (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
  # in cache already
  set(PORTAUDIO_FOUND TRUE)
else (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)

  find_path(PORTAUDIO_INCLUDE_DIR
    NAMES
      portaudio.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )
  
  find_library(PORTAUDIO_LIBRARY
    NAMES
      portaudio
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(PORTAUDIO_INCLUDE_DIRS
    ${PORTAUDIO_INCLUDE_DIR}
  )
  set(PORTAUDIO_LIBRARIES
    ${PORTAUDIO_LIBRARY}
  )

  if (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)
    set(PORTAUDIO_FOUND TRUE)
  endif (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)

  if (PORTAUDIO_FOUND)
    if (NOT Portaudio_FIND_QUIETLY)
      message(STATUS "Found libportaudio: ${PORTAUDIO_LIBRARIES}")
    endif (NOT Portaudio_FIND_QUIETLY)
  else (PORTAUDIO_FOUND)
    if (Portaudio_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libportaudio")
    endif (Portaudio_FIND_REQUIRED)
  endif (PORTAUDIO_FOUND)

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)

endif (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)