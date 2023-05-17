#!/bin/bash
HEIGHT=15
WIDTH=100
CHOICE_HEIGHT=4
BACKTITLE="Eripio encoder v1.5"
TITLE="Choose how the encoder should function"
MENU=""
selectedMenu="root"

currentFolder=$(pwd)
folderOrFile=""
ripperType=""

input1=""
sub1=""
input2=""
sub2=""

fileExtension=""

ffmpegArgs=""
ffmpegSubInputArgs=""
ffmpegSubMetaArgs=""

batchType=""

RunDialog() {
    CHOICE=$(dialog --clear \
        --backtitle "$BACKTITLE" \
        --title "$TITLE" \
        --menu "$MENU" \
        $HEIGHT $WIDTH $CHOICE_HEIGHT \
        "${OPTIONS[@]}" \
        2>&1 >/dev/tty)
}

SubtitleExtractor() {
    subtitleedit /convert "$folderOrFile" subrip

    array[0]=""
    array[1]=""
    i=0
    echo $i
    tmp=${folderOrFile##*/}
    tmp=${tmp%.*}
    while read line
    do
        check=`echo "$line" | grep "$tmp"`

        if [[ -z $check ]];  then 
            echo "";
        else
            check=`echo "$line" | grep ".srt"`

            if [[ -z $check ]];  then 
                echo "";
            else
                echo "Match found";

                echo "$line" | grep "$tmp";
                tmp2=${line#*.};
                tmp2=${tmp2%.*};
                echo "subtitle language is $tmp2";
                array[ $i ]="$tmp2" 
                echo $i
                (( i++ ))
                echo $i
            fi
        fi
    done < <(ls)
    echo $i
    if [ $i == 2 ]; then
        sed -i 's/|/i/gI' "${folderOrFile%.*}.${array[0]}.srt"
        sed -i 's/|/i/gI' "${folderOrFile%.*}.${array[1]}.srt"
        sub1="${folderOrFile%.*}.${array[0]}.srt"
        sub2="${folderOrFile%.*}.${array[1]}.srt"
        input1=" -i"
        input2=" -i"
        ffmpegSubMetaArgs=" -map 1:s -metadata:s:s:0 language=${array[0]} -map 2:s -metadata:s:s:1 language=${array[1]} -c:s copy "
    elif [ $i == 1 ]; then
        sed -i 's/|/i/gI' "${folderOrFile%.*}.${array[0]}.srt"
        sub1="${folderOrFile%.*}.${array[0]}.srt"
        input1=" -i"
        ffmpegSubMetaArgs=" -map 1:s -metadata:s:s:0 language=${array[0]} -c:s copy "

    fi
    
    echo ${array[@]}
    echo $ffmpegSubMetaArgs
}

DetermineFinalArgument() {
    echo ${filenameFormattet%.*}
    if [[ -z $input1 ]]; then
        ffmpeg -i "$folderOrFile" $ffmpegArgs -metadata title="${filenameFormattet%.*}" "${folderOrFile%/*}/0encoded/${filenameFormattet%.*}.$fileExtension"
    elif [[ -z $input2 ]]; then
       ffmpeg -i "$folderOrFile"  $input1 "$sub1" $ffmpegArgs $ffmpegSubMetaArgs -metadata title="${filenameFormattet%.*}" "${folderOrFile%/*}/0encoded/${filenameFormattet%.*}.$fileExtension"
    else
        ffmpeg -i "$folderOrFile" $input1 "$sub1" $input2 "$sub2" $ffmpegArgs $ffmpegSubMetaArgs -metadata title="${filenameFormattet%.*}" "${folderOrFile%/*}/0encoded/${filenameFormattet%.*}.$fileExtension"
    fi
}

ExcecuteRipper() {
    if [ $ripperType == "FFMPEG" ] & [ $batchType == "singlefile" ]; then 
        filenameFormattet=${folderOrFile##*/}
        mkdir 0encoded
        SubtitleExtractor
        DetermineFinalArgument
        
    elif [ $ripperType == "FFMPEG" ] & [ $batchType == "folder" ]; then
        mkdir 0encoded
        for file in *; do  
            if [ -f "$file" ]; then 
                if [ "${file: -4}" == ".mp4" ] || [ "${file: -4}" == ".mkv" ] || [ "${file: -5}" == ".webm" ] || [ "${file: -3}" == ".ts" ]; then
                    folderOrFile=$currentFolder"/"$file
                    filenameFormattet=${folderOrFile##*/}
                    SubtitleExtractor
                    DetermineFinalArgument
                fi 
            fi
        done
    elif [ $ripperType == "FFMPEG" ] & [ $batchType == "subfolder" ]; then
        mkdir 0encoded
        for f in *; do
            if [ -d "$folderOrFile" ] & [ "$folderOrFile" != "0encoded" ]; then
                cd "$currentFolder"
                mkdir 0encoded
                for file in *; do  
                    if [ -f "$file" ]; then 
                        if [ "${file: -4}" == ".mp4" ] || [ "${file: -4}" == ".mkv" ] || [ "${file: -5}" == ".webm" ] || [ "${file: -3}" == ".ts" ]; then
                            folderOrFile=$currentFolder"/"$file
                            filenameFormattet=${folderOrFile##*/}
                            SubtitleExtractor
                            DetermineFinalArgument
                        fi 
                    fi
                done
                cd ..
            fi
        done
    fi
}



MenuContentSwitcher () {
    case $selectedMenu in
        "root")
            # Root menu
            OPTIONS=(1 "Single file"
                     2 "Folder"
                     3 "Folder and subfolder (UNTESTED)")

            RunDialog

            case $CHOICE in
                    1)  
                        folderOrFile=$(dialog --fselect "$currentFolder" 15 100 2>&1 >/dev/tty)
                        selectedMenu="FFMPEG"
                        batchType="singlefile"
                    ;;
                    
                    2)
                        folderOrFile=$currentFolder
                        selectedMenu="FFMPEG"
                        batchType="folder"
                    ;;

                    3)
                        folderOrFile=$currentFolder
                        selectedMenu="FFMPEG"
                        batchType="subfolder"
                    ;;
                
            esac
            MenuContentSwitcher
        ;;
        "FFMPEG")
            OPTIONS=(1 "no aspectratio change" 
                        2 "1.78:1 crop"
                        3 "1.85:1 crop"
                        4 "2.00:1 crop"
                        5 "2.35:1 crop"
                        6 "2.39:1 crop"
                        7 "2.40:1 crop")
            
            RunDialog
            fileExtension="mkv"
            ripperType="FFMPEG"


            case $CHOICE in
                    1)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -codec:a eac3 -map -0:s"
                    ;;
                    
                    2)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:1080 -codec:a eac3 -map -0:s"
                    ;;
                    
                    3)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:1036 -codec:a eac3 -map -0:s"
                    ;;

                    4)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:960 -codec:a eac3 -map -0:s"
                    ;;

                    5)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:814 -codec:a eac3 -map -0:s"
                    ;;
                    
                    6)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:802 -codec:a eac3 -map -0:s"
                    ;;
                    
                    7)
                        ffmpegArgs=" -map 0 -codec:v libx265 -crf 21 -filter:v crop=1920:800 -codec:a eac3 -map -0:s"
                    ;;
            esac
            ExcecuteRipper
            
        ;;
        
    esac



}

MenuContentSwitcher