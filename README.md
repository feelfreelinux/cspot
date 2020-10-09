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
- ffmpeg for playback on the desktop CLI version
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
$ mkdir -p build && cd build

# use cmake to generate build files
$ cmake ..

# compile
$ make 
```
See running the CLI for information on how to run cspot on a desktop computer.

## Running

## The CLI version

Currently the CLI version outputs the raw audio data to a FIFO-file named outputFifo. It has to be read by ffmpeg running as another process to get audio playback on a desktop computer. Portaudio output coming soon.

```shell
# execute these comands in the directory where cspotcli was built.

# creates a FIFO
$ mkfifo outputFifo

# run ffmpeg in an infinite loop to play the audio
$ while true; ffplay  -volume 20 -autoexit -f s16le -ac 2 -i outputFifo; end

# now in another terminal window you can finally run cspot
$ ./cspotcli "spotify_username" "spotify_password"

```

Now open a real Spotify app and you should see a cspot device on your local network. Use it to play audio.

Please not that if you use facebook login you can still generate a password on spotify.


