![C/C++ CI](https://github.com/feelfreelinux/cspot/workflows/C/C++%20CI/badge.svg)
![ESP IDF](https://github.com/feelfreelinux/cspot/workflows/ESP%20IDF/badge.svg)
[![Certification](https://badgen.net/badge/Stary%20Filipa/certified?color=purple)](https://github.com/feelfreelinux/cspot)
[![Certification](https://badgen.net/badge/Memory%20leaks/yes)](https://github.com/feelfreelinux/cspot)
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

- cmake (version 3.18 or higher)
- gcc / clang for the CLI target
- [esp-idf](https://github.com/espressif/esp-idf) for building for the esp32
- portaudio for playback on MacOS
- downloaded submodules
- python libraries for the [nanopb](https://github.com/nanopb/nanopb) generator
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

# use cmake to generate build files
$ cmake ..

# compile
$ make 
```
See running the CLI for information on how to run cspot on a desktop computer.

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

- [`ALSAAudioSink`](targets/cli/ALSAAudioSink.cpp) - Linux, requires `libasound`
- [`PortAudioSink`](targets/cli/PortAudioSink.cpp) - MacOS (PortAudio also supports more platforms, but we currently use it only on MacOS), requires the PortAudio library
- [`NamedPipeAudioSink`](targets/cli/NamedPipeAudioSink.cpp) - all platforms, writes to a file/FIFO pipe called `outputFifo` which can later be played back by FFmpeg. Used mainly for testing and development.

Additionaly the following audio sinks are implemented for the esp32 target:
- [`ES9018AudioSink`](targets/esp32/main/sinks/ES9018AudioSink.cpp) - provides playback via the ES9018 DAC connected to the ESP32
- [`AC101AudioSink`](targets/esp32/main/sinks/AC101AudioSink.cpp) - provides playback via AC101 DAC used in cheap ESP32 A1S audiokit boards, commonly found on aliexpress.
- TODO: internal esp32 DAC for crappy quality testing.

You can also easily add support for your own DAC of choice by implementing your own audio sink. Each new audio sink must implement the `void feedPCMFrames(std::vector<uint8_t> &data)` method which should accept stereo PCM audio data at 44100 Hz and 16 bits per sample. Please note that the sink should somehow buffer the data, because playing it back may result in choppy audio.

An audio sink can optionally implement the `void volumeChanged(uint16_t volume)` method which is called everytime the user changes the volume (for example via Spotify Connect). If an audio sink implements it it should set `softwareVolumeControl` to `false` in its consructor to let cspot know to disable the software volume adjustment. Properly implementing external volume control (for example via dedicated hardware) will result in a better playback quality since all the dynamic range is used to encode the samples.

The embedding program should also handle caching the authentication data, so that the user does not have to authenticate via the local network (Zeroconf) each time cspot is started. For reference on how to do it please refer to the `cspot-cli` target (It stores the data in `authBlob.json`). 

## Internal details

The connection with Spotify servers to play music and recieve control information is pretty complex. First of all an access point address must be fetched from Spotify ([`ApResolve`](cspot/src/ApResolve.cpp) fetches the list from http://apresolve.spotify.com/). Then a [`PlainConnection`](cspot/include/PlainConnection.h) with the selected Spotify access point must be established. It is then upgraded to an encrypted [`ShannonConnection`](cspot/include/ShannonConnection.h).
