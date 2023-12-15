# Eripio Encoder

A simple frontend for FFMPEG written in c++ that batch converts .mkv files.

## Needed packages:

- FFMPEG 
- subtitleedit-cli or subtitleedit
    - tesseract ocr data packages (specific for the languages you want to convert)
- raylib (for image processing)

## Compilation:
 ```g++ -lraylib -Wall -Werror src/main.cpp -o build/encoder```

## Extra info

- if you install raylib via your package manager, you will need to recompile the program after updating raylib
