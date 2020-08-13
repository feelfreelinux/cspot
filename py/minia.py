# This is an example of the use of PyOgg.
#
# Author: Matthew Walker 2020-06-01
#
# An Ogg Opus file (a file containing an Opus stream wrapped inside an
# Ogg container) is loaded using the Opusfile library.  This provides
# the entire file in one PCM encoded buffer.  That buffer is converted
# to a NumPy array and then played using simpleaudio.
#
# On successful execution of this program, you should hear the audio
# being played and the console will display comething like:
#
#    Reading Ogg Opus file...
# 
#    Read Ogg Opus file
#    Channels: 2
#    Frequency (samples per second): 48000
#    Buffer Length (bytes): 960000
#    Shape of numpy array (number of samples per channel, number of channels): (240000, 2)
#    
#    Playing...
#    Duration: 0:00:05.190433
#    Finished.


try:
    import pyogg
    import simpleaudio as sa
    import numpy
except ImportError:
    import os
    should_install_requirements = input(\
        "This example requires additional libraries to work.\n" +
        "  py-simple-audio (simpleaudio),\n" +
        "  NumPy (numpy)\n" +
        "  And PyOgg or course.\n" +
        "Would you like to install them right now?\n"+
        "(Y/N): ")
    if should_install_requirements.lower() == "y":
        import subprocess, sys
        
        install_command = sys.executable + " -m pip install -r " + \
                              os.path.realpath("01-play-opus-simpleaudio.requirements.txt")

        popen = subprocess.Popen(install_command,
                                 stdout=subprocess.PIPE, universal_newlines=True)
        
        for stdout_line in iter(popen.stdout.readline, ""):
            print(stdout_line, end="")
            
        popen.stdout.close()
        
        popen.wait()

        print("Done.\n")

        import pyogg
        import simpleaudio as sa
        import numpy

    else:
        os._exit(0)
        
import ctypes
from datetime import datetime


# Specify the filename to read
filename = "dupa.ogg"


# Read the file using OpusFile
print("Reading Ogg Opus file...")
opus_file = pyogg.VorbisFile(filename)


# Display summary information about the audio
print("\nRead Ogg Opus file")
print("Channels:\n  ", opus_file.channels)
print("Frequency (samples per second):\n  ",opus_file.frequency)
print("Buffer Length (bytes):\n  ", opus_file.buffer_length)


# Using the data from the buffer in OpusFile, create a NumPy array
# with the correct shape.  Note that this does not copy the buffer's
# data.
bytes_per_sample = ctypes.sizeof(opus_file.buffer.contents)
buffer = numpy.ctypeslib.as_array(
    opus_file.buffer,
    (opus_file.buffer_length//
     bytes_per_sample//
     opus_file.channels,
     opus_file.channels)
)


# The shape of the array can be read as
# "(number of samples per channel, number of channels)".
print("Shape of numpy array (number of samples per channel, number of channels):\n  ",
      buffer.shape)


# Play the audio
print("\nPlaying...")
start_time = datetime.now()
play_obj = sa.play_buffer(buffer,
                          opus_file.channels,
                          bytes_per_sample,
                          opus_file.frequency)


# Wait until sound has finished playing
play_obj.wait_done()  


# Report on the time spent during playback
end_time = datetime.now()
print("Duration: "+str(end_time - start_time))
print("Finished.")