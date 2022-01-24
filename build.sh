#!/usr/bin/env bash

set -e
set -u

INSTALL_PREFIX=$PWD

mkdir -p generated

mkdir -p build
cd build

cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
make
make install
