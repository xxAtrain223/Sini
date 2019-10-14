#!/usr/bin/env bash
set -e
mkdir build
cd build

cmake -Dsini_ENABLE_TESTING:BOOL=ON ..