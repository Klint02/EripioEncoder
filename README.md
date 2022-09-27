# Eripio Ripper

A simple collection of Bash scripts I use to fix my videofiles when I rip them from Blu Rays, DVD and other media.

Simply launch the script in the folder where your files or subfolders with files are located, choose the desired action and let it rip.

For easier execution, add it to your bashrc, PATH or simply create an Alias that points to the global path.

I might add more scripts to the collection as I continue to rip more media.

## Needed packages for this collection:

- Rename (for mass renaming)
- FFMPEG (container fixing and batch converting to mkv)
- MKVpropedit (fixing mkv metadata)
- MKVextract (extracting image subs)
- Subtitleedit (converting image subs to SRT subs)


## Supported platforms:
- Anything that runs bash, I think...

## Extra stuffs

- I will not be supporting zsh, fish, normal shell etc. but you could maybe modify the scripts to do so.
- VobSubs aren't supported right now, but is easy to support if you need it.
