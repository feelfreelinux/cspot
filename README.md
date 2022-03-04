![C/C++ CI](https://github.com/feelfreelinux/cspot/workflows/C/C++%20CI/badge.svg)
![ESP IDF](https://github.com/feelfreelinux/cspot/workflows/ESP%20IDF/badge.svg)
[![Certification](https://badgen.net/badge/Stary%20Filipa/certified?color=purple)](https://github.com/feelfreelinux/cspot)
[![Certification](https://badgen.net/badge/Sasin/stole%2070%20mln%20PLN)](https://github.com/feelfreelinux/cspot)

<p align="center">
<img src=".github/trombka.png" width="32%" />
</p>

# :trumpet: cspot

A Spotify Connect player written in CPP targeting, but not limited to embedded devices (ESP32).

Currently in state of rapid development.

*Only to be used with premium spotify accounts*

## Building

### Prerequisites

Summary:

- cmake (version 3.0 or higher)
- gcc / clang for the CLI target
- [esp-idf](https://github.com/espressif/esp-idf) for building for the esp32
- portaudio for playback on MacOS
- downloaded submodules
- golang (1.16)
- protoc
- on Linux you will additionally need:
    - `libasound` and `libavahi-compat-libdnssd`


This project utilizes submodules, please make sure you are cloning with the `--recursive` flag or use `git submodule update --init --recursive`.

This library uses nanopb to generate c files from protobuf definitions. Nanopb itself is included via submodules, but it requires a few external python libraries to run the generators.

To install them you can use pip:

```shell
$ sudo pip3 install protobuf grpcio-tools
```

(You probably should use venv, but I am no python developer)

To install avahi and asound dependencies on Linux you can use:

```shell
$ sudo apt-get install libavahi-compat-libdnssd-dev libasound2-dev
```


### Building for macOS/Linux

The cli target is used mainly for testing and development purposes, as of now it has the same features as the esp32 target.

```shell
# navigate to the targets/cli directory
$ cd targets/cli

# create a build directory and navigate to it
$ mkdir -p build && cd build

# use cmake to generate build files, and select an audio sink
$ cmake .. -DUSE_PORTAUDIO=ON

# compile
$ make 
```
See running the CLI for information on how to run cspot on a desktop computer.

### Building for ESP32

The ESP32 target is built using the [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) toolchain

```shell
# update submodules after each code pull to avoid build errors
$ git submodule update
# navigate to the targets/esp32 directory
$ cd targets/esp32
# run once after pulling the repo
$ idf.py set-target esp32
```

Configure CSPOT according to your hardware

```shell
# run visual config editor, when done press Q to save and exit
$ idf.py menuconfig
```

Navigate to `Example Connection Configuration` and provide wifi connection details

![idf-menuconfig](/targets/esp32/doc/img/idf-menuconfig-2.png)

Navigate to `CSPOT Configuration`, you may configure device name, output device and audio quality.

![idf-menuconfig](/targets/esp32/doc/img/idf-menuconfig-1.png)

#### Status LED

By default LED indication is disabled, but you can use either standard GPIO or addressable LED to indicate cspot current status. It will use different blinking patterns (and colors in case of addressable LEDs) to indicate Wifi connectivity and presense of connected Spotify client.

#### Building and flashing

Build and upload the firmware

```shell
# compile
$ idf.py build

# upload
$ idf.py flash
```
The ESP32 will restart and begin running cspot. You can monitor it using a serial console.

Optionally run as single command

```shell
# compile, flash and attach monitor
$ idf.py build flash monitor
```

## Running

## The CLI version

After building the app, the only thing you need to do is to run it through CLI.

```shell
$ ./cspotcli

```

Now open a real Spotify app and you should see a cspot device on your local network. Use it to play audio.


# Architecture

## External interface

`cspot` is meant to be used as a lightweight C++ library for playing back Spotify music and receive control notifications from Spotify connect. 
It exposes an interface for starting the communication with Spotify servers and expects the embedding program to provide an interface for playing back raw audio samples ([`AudioSink`](include/AudioSink.h)).

You can view the [`cspot-cli`]([targets/cli/main.cpp) program for a reference on how to include cspot in your program. It provides a few audio sinks for various platforms and uses:

- [`ALSAAudioSink`](cspot/bell/src/sinks/unix/ALSAAudioSink.cpp) - Linux, requires `libasound`
- [`PortAudioSink`](cspot/bell/src/sinks/unix/PortAudioSink.cpp) - MacOS (PortAudio also supports more platforms, but we currently use it only on MacOS), requires the PortAudio library
- [`NamedPipeAudioSink`](cspot/bell/src/sinks/unix/NamedPipeAudioSink.cpp) - all platforms, writes to a file/FIFO pipe called `outputFifo` which can later be played back by FFmpeg. Used mainly for testing and development.

Additionaly the following audio sinks are implemented for the esp32 target:
- [`ES9018AudioSink`](cspot/bell/src/sinks/esp/ES9018AudioSink.cpp) - provides playback via a ES9018 DAC connected to the ESP32
- [`AC101AudioSink`](cspot/bell/src/sinks/esp/AC101AudioSink.cpp) - provides playback via the AC101 DAC used in cheap ESP32 A1S audiokit boards, commonly found on aliexpress.
- [`PCM5102AudioSink`](cspot/bell/src/sinks/esp/PCM5102AudioSink.cpp) - provides playback via a PCM5102 DAC connected to the ESP32, commonly found in the shape of small purple modules at various online retailers. Wiring can be configured in the sink and defaults to:
  - SCK to Ground
  - BCK to PGIO27
  - DIN to GPIO25
  - LCK to GPIO32
  - GND to Ground
  - VIN to 3.3V (but supposedly 5V tolerant)
- TODO: internal esp32 DAC for crappy quality testing.

You can also easily add support for your own DAC of choice by implementing your own audio sink. Each new audio sink must implement the `void feedPCMFrames(std::vector<uint8_t> &data)` method which should accept stereo PCM audio data at 44100 Hz and 16 bits per sample. Please note that the sink should somehow buffer the data, because playing it back may result in choppy audio.

An audio sink can optionally implement the `void volumeChanged(uint16_t volume)` method which is called everytime the user changes the volume (for example via Spotify Connect). If an audio sink implements it it should set `softwareVolumeControl` to `false` in its consructor to let cspot know to disable the software volume adjustment. Properly implementing external volume control (for example via dedicated hardware) will result in a better playback quality since all the dynamic range is used to encode the samples.

The embedding program should also handle caching the authentication data, so that the user does not have to authenticate via the local network (Zeroconf) each time cspot is started. For reference on how to do it please refer to the `cspot-cli` target (It stores the data in `authBlob.json`). 

## Internal details

The connection with Spotify servers to play music and recieve control information is pretty complex. First of all an access point address must be fetched from Spotify ([`ApResolve`](cspot/src/ApResolve.cpp) fetches the list from http://apresolve.spotify.com/). Then a [`PlainConnection`](cspot/include/PlainConnection.h) with the selected Spotify access point must be established. It is then upgraded to an encrypted [`ShannonConnection`](cspot/include/ShannonConnection.h).
