***WARNING***: This repo is out of date, and still uses Toy v1. I'm currently rewriting Toy as v2, and this will receive a similar treatment - but I'm not there yet.

# The Box Engine

This game engine utilizes the [Toy Programming Language](https://toylang.com).

## Cloning

Either clone recursively, or run `git submodule update --init` after cloning.

## Building

For Windows(mingw32 & cygwin), Linux and MacOS, simply run `make` in the root directory.

For Windows(MSVC), Visual Studio project files are included.

Note: MacOS and Windows(MSVC) are not officially supported, but we'll do our best!

## Running

Make sure the program can see the `assets` folder (symbolic links can help), and all of the required DLLs are present.

## Dependencies

* SDL2
* SDL2_image
* SDL2_mixer
* SDL2_ttf
* libcurses (for debugging only)

# License

The source code for this project, not including the contents of Toy, is covered by the zlib license (see [LICENSE.md](LICENSE.md)).

# Patrons via Patreon

* Seth A. Robinson
