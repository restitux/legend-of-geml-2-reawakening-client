#!/usr/bin/env bash

set -e

cd dist

python3 -m http.server
