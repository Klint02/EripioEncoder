# Eripio Encoder

A simple frontend for FFMPEG written in c++ that batch converts .mkv files.

## Needed packages:

- FFMPEG 
- subtitleedit-cli or subtitleedit
    - tesseract ocr data packages (specific for the languages you want to convert)
- mkvtoolnix-cli (metadata editing during remuxing)
- raylib (for image processing)

## Compilation:
```
mkdir build

cd build

cmake ..

make
```

## Usage:
Run scan and load seperately by using: `-s` and `-l`. 

I recommend scanning the movies first, checking the movies.txt that is generated for any errors, and then running the load function.

Running the program with `-s -l -c` disables the scanner and encoder, and runs verification of the movies that has been encoded. The verification checks if the duration of the encoded movie is within two seconds of the original. Keep in mind that ffmpeg may add four seconds to the duration and thus making the verification fail. In these cases just ignore the errors. 

## Extra info

- If you install raylib via your package manager, you will need to recompile the program after updating raylib.
- For movies with two or more subtitles of the same language, it is recommended to seperate the .srt files and removing the extra language from the subtitle part of the movies.txt.
