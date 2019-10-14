#!/usr/bin/env bash
set -e

pushd build
make -j2

./siniTest

popd

cppcheck include/*.hpp