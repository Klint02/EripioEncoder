#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "encoder.h"

int main()
{
    Encoder encoder;
    std::string current_path = fs::current_path();
    for (const auto& entry : fs::directory_iterator(current_path)) 
    {
        encoder.run(entry);
        std::cout << entry << std::endl;
        encoder.video.subtitle_count = 2;
        //encoder.
    }

/*
    FILE *p = popen("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of default=nw=1:nk=1" , "r");

    if (p != NULL) {
        std::cout << "running" << std::endl;
    }
*/
    //pclose(p);
}

