#!/bin/bash

HEIGHT=15
WIDTH=100
CHOICE_HEIGHT=4
BACKTITLE="Eripio Ripper v1.0"
TITLE="Video scripts"
MENU="Choose a script to launch:"

OPTIONS=(1 "Change video container for folder (preserve sub)"
         2 "Change video container for folder"
         3 "Change video container for folder and subfolder"
         4 "MKV Cleaner for folder 1 sub included (eng sub)"
         5 "MKV Cleaner for folder 2 sub included"
         6 "MKV Cleaner for folder"
         7 "MKV Cleaner for folder and subfolder 1 sub included (eng sub)"
         8 "MKV Cleaner for folder and subfolder"
         9 "Subremove for folder"
         10 "Subremove for folder and subfolder"
         11 "Merge DA and EN subs into mkv and clean"
         12 "Batch encode crf 23 and vorbis folder and subfolder"
         13 "Batch encode crf 23 and vorbis"
         14 "Sub extraction (Sub 1)"
         15 "Sub extraction (Sub 2)"
         16 "MKV Cleaner for anime folder 2 sub included (1 sign song 2 full subs)"
         17 "MKV Cleaner for anime folder 2 sub included (1 full subs 2 sign song)"
         18 "MKV Cleaner for anime folder 1 sub included (1 full subs)"
         19 "MKV Cleaner for anime folder 1 sub included (1 sign song)"
         20 "Rename sub for series (Eng sub)"
         21 "Convert PGS subs on track 2 of an mkv to SRT for folder")

CHOICE=$(dialog --clear \
                --backtitle "$BACKTITLE" \
                --title "$TITLE" \
                --menu "$MENU" \
                $HEIGHT $WIDTH $CHOICE_HEIGHT \
                "${OPTIONS[@]}" \
                2>&1 >/dev/tty)

clear
case $CHOICE in
        1)
            for file in *; do  
                if [ -f "$file" ]; then 
                    if [ "${file: -4}" == ".mp4" ] || [ "${file: -3}" == ".ts" ]; then
                        ffmpeg -i "$file" -c copy -c:s srt -c:s srt "${file%.*}.mkv"
                    fi 
                fi
            done
            ;;
        2)
            for file in *; do  
                if [ -f "$file" ]; then 
                    if [ "${file: -4}" == ".mp4" ] || [ "${file: -3}" == ".ts" ]; then
                        ffmpeg -i "$file" -c copy "${file%.*}.mkv"
                    fi 
                fi
            done
            ;;
        3)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    for file in *; do  
                        if [ -f "$file" ]; then 
                            if [ "${file: -4}" == ".mp4" ] || [ "${file: -3}" == ".ts" ]; then
                                ffmpeg -i "$file" -c copy "${file%.*}.mkv"
                            fi 
                        fi
                    done
                    cd ..
                fi
            done
            ;;
       4)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:s1 --set name="" --set language="Eng"
                fi 
            done
            ;;
        5)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:s1 --set name="" --edit track:s2 --set name=""
                fi 
            done
            ;;
        6)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name=""
                fi 
            done
            ;;
            
        7)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    for file in *; do  
                        if [ -f "$file" ]; then 
                            if [ "${file: -4}" == ".mkv" ]; then
                                mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:s1 --set name="" --set language="Eng"
                            fi 
                        fi
                    done
                    cd ..
                fi
            done
            ;;
        8)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    for file in *; do  
                        if [ -f "$file" ]; then 
                            if [ "${file: -4}" == ".mkv" ]; then
                                mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" 
                            fi 
                        fi
                    done
                    cd ..
                fi
            done
            ;;
        9)
            mkdir removed
            for file in *; do 
                if [ -f "$file" ]; then 
                    ffmpeg -i "$file" -sn -c copy "removed/${file%.*}.mkv"
                fi 
            done
            ;;
        10)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    mkdir removed
                    for file in *; do  
                        if [ -f "$file" ]; then 
                            if [ "${file: -4}" == ".mkv" ]; then
                                ffmpeg -i "$file" -sn -c copy "removed/${file%.*}.mkv"
                            fi 
                        fi
                    done
                    cd ..
                fi
            done
            ;;
        11)
            mkdir output
            for file in *; do 
                if [ -f "$file" ]; then 
                    if [ "${file: -4}" == ".mkv" ]; then
                    ffmpeg -i "$file" -i "${file%.*}.da.srt" -i "${file%.*}.en.srt" -map 0:v -map 0:a -map 1:s -map 2:s -c copy "output/$file"
                    fi
                fi 
            done


            cd output

            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:s1 --set language="Dan" --edit track:s2 --set language="Eng"  
                fi 
            done
            ;;
        12)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    mkdir encoded
                    for file in *; do  
                        if [ -f "$file" ]; then 
                                ffmpeg -i "$file" -codec:v libx265 -crf 23 -codec:a libvorbis -qscale:a 5 "encoded/${file%.*}.mkv"
                        fi
                    done
                    cd ..
                fi
            done
            ;;
        13)
            mkdir encoded
            for file in *; do 
                if [ -f "$file" ]; then 
                    ffmpeg -i "$file" -codec:v libx265 -crf 23 -codec:a libvorbis -qscale:a 5 "encoded/${file%.*}.mkv"
                fi 
            done
            ;;
        14)
            for file in *; do 
                if [ -f "$file" ]; then 
                    ffmpeg -i "$file" -map 0:2 "${file%.*}.srt"
                fi 
            done
            ;;
        15)
            for file in *; do 
                if [ -f "$file" ]; then 
                    ffmpeg -i "$file" -map 0:3 "${file%.*}.srt"
                fi 
            done
            ;;
        16)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:a2 --set name="" --edit track:s1 --set language="Eng" --set name="sign song" --edit track:s2 --set language="Eng" --set name="full subs"
                fi 
            done
            ;;
        17)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:a2 --set name="" --edit track:s1 --set language="Eng" --set name="full subs" --edit track:s2 --set language="Eng" --set name="sign song"
                fi 
            done
            ;;
        18)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:a2 --set name="" --edit track:s1 --set language="Eng" --set name="full subs"
                fi 
            done
            ;;
        19)
            for file in *; do 
                if [ -f "$file" ]; then 
                    mkvpropedit "$file" --edit info --set title="${file%.*}" --edit track:v1 --set name="" --edit track:a1 --set name="" --edit track:a2 --set name="" --edit track:s1 --set language="Eng" --set name="sign song"
                fi 
            done
            ;;
        20)
            for f in *; do
                if [ -d "$f" ]; then
                    cd "$f"
                    rename .srt .en.srt *
                    cd ..
                fi
            done
            echo "done"
            ;;
        21)
            for file in *; do 
                if [[ -f "${file%.*}.mkv" && ! -f "${file%.*}.srt" ]]; then 
                    mkvextract "$file" tracks 2:"${file%.*}.pgs"
                    subtitleedit /convert "${file%.*}.pgs" subrip
                fi 
            done
            rm -rf *.pgs
            ;;
esac

