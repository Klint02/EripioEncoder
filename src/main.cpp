#include <iostream>
#include <filesystem>
#include <array>
#include <unordered_map>
#include <vector>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

//#include "encoder.cpp"

struct Video_file {
    std::string path;
    std::string video_title;

    std::vector<std::string> subtitle_langs;

    int audio_track_count;

    int original_height;
    int original_width;

    int width;
    int height;
    
};

bool contains(int len, char** arr, std::string str1) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == str1) {
            return true;
        }
    }
    return false;
}

bool contains(int len, char** arr, std::string str1, std::string str2) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == str1 || arr[i] == str2) {
            return true;
        }
    }
    return false;
}

std::string cmd_exec(std::string arg, std::string flag) {
    char output_buffer [1024];
    FILE *process = popen(arg.c_str() , "r");
    if(process != NULL) {
        while(fgets(output_buffer, sizeof(output_buffer), process) != NULL) {
            if (flag == "verbose" || flag == "v") std::cout << output_buffer << std::endl;
            
        }
    }
    pclose(process);
    return output_buffer;
}


void convert_subtitles(std::unordered_map<std::string, Video_file>* movies) {
/*     for (auto& movie : *movies) {
        //cmd_exec("subtitleedit /convert \"" + movie.second.path + "\" subrip", "v");
    } */
}

int main(int argc, char** argv)
{
    std::unordered_map<std::string, Video_file> movies;

    std::string path = fs::current_path();
    
    if (contains(argc, argv, "-h", "--help")) {
        std::cout 
        << "Welcome to the encoder \n" 
        << "    -p or --path \t Specify path for encoding \n"
        << "    -h or --help \t This message"
        << std::endl;
    } else {
        if  (contains(argc, argv, "-p", "--path")) {
            for (int i = 0; i < argc; i++) {
                if (((std::string)argv[i] == "-p" || (std::string)argv[i] == "--path")) {
                    if (argc - 1 != i) {
                        path = argv[i + 1];
                        if (path[0] == '~') {
                            path = std::getenv("HOME") + path.substr(1);
                        }
                    } else {
                        std::cout << "No path given" << std::endl;
                        return 0;
                    }
                }
            }
        }
        if (contains(argc, argv, "-a", "--async")) {
            std::cout << "Not Implemented" << std::endl;
            return 0;
        }

    }

    std::cout << "[INFO] Determining audio tracks for movies" << std::endl;
    for (const auto& entry : fs::directory_iterator(path)) 
    {
        if (entry.path().extension() == ".mkv" || entry.path().extension() == ".mp4") {

            Video_file movie;
            movie.path = entry.path().string();
            movie.video_title = entry.path().filename();
            std::string out = cmd_exec("ffprobe -v error -select_streams a -show_entries stream=codec_type -of default=nw=1:nk=1 \"" + movie.path + "\" | uniq -c", "");
            out.erase(remove_if(out.begin(), out.end(), isspace), out.end());
            movie.audio_track_count = stoi(out.substr(0,1));
            movies.insert({entry.path().filename(), movie});

        }
    }
    
/*     for (auto& movie : movies) {
        std::cout << movie.second.video_title << " " << movie.second.audio_track_count << std::endl;
        //cmd_exec("subtitleedit /convert \"" + movie + "\" subrip");
    } */

    std::cout << "Starting subtitle conversion thread" << std::endl;
    std::thread subtitle_thread (convert_subtitles, &movies);

    subtitle_thread.join();

    for (auto& movie : movies) {
        std::cout << movie.second.video_title << " " << movie.second.audio_track_count << std::endl;
        //cmd_exec("subtitleedit /convert \"" + movie + "\" subrip");
    } 
    return 0;

/*     Encoder encoder;
    encoder.store_video_paths(); */
    
    //encoder.find_resolution("/home/klint/Git/EripioEncoder/testData/2.40:1/img003.png");
    /*
    std::string current_path = fs::current_path();
    for (const auto& entry : fs::directory_iterator(current_path)) 
    {
        encoder.run(entry);
        std::cout << entry << std::endl;
        encoder.video.subtitle_count = 2;
        //encoder.
    }
*/
/*
    FILE *p = popen("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of default=nw=1:nk=1" , "r");

    if (p != NULL) {
        std::cout << "running" << std::endl;
    }
*/
    //pclose(p);
}

