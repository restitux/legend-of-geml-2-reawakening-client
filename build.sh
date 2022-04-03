#!/bin/bash

set -e

rm -r dist
mkdir dist
mkdir dist/res

cp -r web/* dist/
cp -r res/* dist/res

zig build -Dtarget=wasm32-freestanding -Drelease-small=true

emcc zig-out/lib/libgamejam.a -s WASM=1 -s USE_SDL=2 -s FETCH=1 -O3 -o dist/index.js