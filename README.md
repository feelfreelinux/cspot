![C/C++ CI](https://github.com/feelfreelinux/cspot/workflows/C/C++%20CI/badge.svg)
![ESP IDF](https://github.com/feelfreelinux/cspot/workflows/ESP%20IDF/badge.svg)
[![Commits](https://badgen.net/github/commits/feelfreelinux/cspot?color=red)](https://github.com/feelfreelinux/cspot)
[![Last commmit](https://badgen.net/github/last-commit/feelfreelinux/cspot?color=orange)](https://github.com/feelfreelinux/cspot)
[![Forks](https://badgen.net/github/forks/feelfreelinux/cspot?color=yellow)](https://github.com/feelfreelinux/cspot)
[![Stars](https://badgen.net/github/stars/feelfreelinux/cspot?color=green)](https://github.com/feelfreelinux/cspot)
[![Open issues](https://badgen.net/github/open-issues/feelfreelinux/cspot?color=blue)](https://github.com/feelfreelinux/cspot)
[![Certification](https://badgen.net/badge/Stary%20Filipa/certified?color=purple)](https://github.com/feelfreelinux/cspot)
[![Certification](https://badgen.net/badge/Memory%20leaks/yes)](https://github.com/feelfreelinux/cspot)
[![Certification](https://badgen.net/badge/Sasin/stole%2070%20mln%20PLN)](https://github.com/feelfreelinux/cspot)



# cspot

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

This project utilizes submodules, please make sure you are cloning with the `--recursive` flag or use `git submodule update --init --recursive`.

This library uses nanopb to generate c files from protobuf definitions. Nanopb itself is included via submodules, but it requires a few external python libraries to run the generators.

To install them you can use pip:

```shell
$ sudo pip3 install protobuf grpcio-tools
```

(You probably should use venv, but I am no python developer)

### Building for macOS/Linux

The cli target is used mainly for testing and development purposes, as of now it has the same features as the esp32 target.

```shell
# navigate to the targets/cli directory
$ cd targets/cli

# create a build directory and navigate to it
$ mkdir build && cd build

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

