#!/bin/sh

clear

echo "[INFO] Removing old binary"
rm build/encoder

echo "[INFO] Compiling new binary"
g++ -lraylib -Wall -Werror src/main.cpp -o build/encoder && ./run.sh
