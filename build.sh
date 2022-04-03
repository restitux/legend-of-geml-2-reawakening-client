#!/bin/bash

set -e

rm -rf dist
mkdir dist
mkdir dist/res

cp -r web/* dist/
cp -r res/* dist/res

zig build -Dtarget=wasm32-freestanding -Drelease-small=true

emcc zig-out/lib/libgamejam.a -s WASM=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s FETCH=1 -O3 -o dist/index.js
