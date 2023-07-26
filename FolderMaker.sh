#!/bin/bash

for file in *; do  
    if [ -f "$file" ]; then 
        if [ "${file: -4}" == ".mkv" ]; then
            filenameFormattet=${file%.*}
            mkdir "$filenameFormattet"
            mv "$file" "$filenameFormattet/$file"
        fi 
    fi
done