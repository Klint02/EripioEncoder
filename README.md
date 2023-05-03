# Eripio Encoder

A simple frontend for FFMPEG that can take a single file, folder or a folder's subfolders.

Simply launch the script choose the file or launch the script inside the folders you want to encode. It will by default only support changing aspect ratios for 1080p video files.

The encoder will convert PGS and VOBSUB to srt and fix a common error where "I" would be "|". After converting the subtitles, the script will then embed them inside the .mkv file. 

The script will by default only support upto two subtitle streams, but adding support for more is as simple as changing the `DetermineFinalArgument()` and `SubtitleExtractor()` to handle more than two subs.

The purpose of the script is to make encoding blu rays and DVDs easier.

For easier execution, add it to your bashrc, PATH or simply create an Alias that points to the global path.

## Needed packages for this collection:

- FFMPEG 
- subtitleedit-cli or subtitleedit
    - tesseract ocr data packages (specific for the languages you want to convert)


## Supported platforms:
- Anything that runs bash, I think...

## Extra stuffs

- I will not be supporting zsh, fish, normal shell etc. but you could maybe modify the scripts to do so.
