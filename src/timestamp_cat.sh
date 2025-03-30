#!/bin/bash

echo "" > stamp.txt


for file in *; do  
    if [ -f "$file" ]; then 
        echo "$file" >> stamp.txt
        ffprobe -v error -select_streams v:0 -show_entries chapter=end_time,title -of default=noprint_wrappers=1:nokey=1 "$file" >> stamp.txt
    fi
done
