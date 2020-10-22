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
    - libasound and libavahi-compat-libdnssd


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



