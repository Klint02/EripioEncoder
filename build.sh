#!/bin/sh

clear

echo "[INFO] Removing old binary"
rm build/encoder

echo "[INFO] Compiling new binary"
g++ -lraylib -std=c++20 -Wall -Werror src/main.cpp -o build/encoder && ./run.sh
