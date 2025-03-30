#!/bin/bash

for file in *; do  
    if [ -f "$file" ]; then 
        if [ "${file: -4}" == ".mkv" ]; then
            filenameFormattet=${file%.*}
            mkdir "$filenameFormattet"
            mv "$file" "$filenameFormattet/$file"
        fi 
        if [ "${file: -4}" == ".ISO" ]; then
            filenameFormattet=${file%.*}
            mkdir -p "$filenameFormattet"
            mv "$filenameFormattet".* "$filenameFormattet/"
        fi
        if [ "${file: -4}" == ".iso" ]; then
            filenameFormattet=${file%.*}
            mkdir -p "$filenameFormattet"
            mv "$filenameFormattet".* "$filenameFormattet/"
        fi
    fi
done
