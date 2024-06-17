#!/bin/sh

set -e
mkdir -p build

clang src/brainfuck.c -Wall -Wpedantic -O3 -march=native -o build/brainfuck
