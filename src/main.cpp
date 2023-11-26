#include <iostream>
#include <filesystem>
#include <array>
#include <unordered_map>
#include <vector>
#include <thread>
#include <algorithm>
#include <format>

namespace fs = std::filesystem;

//#include "encoder.cpp"
struct Program_status {
    int movie_count = 0;
    int subtitle_tack_progress = 0;
    int audio_track_progress = 0;
    int compression_progress = 0;

    bool subtitle_thread_running = false;
    bool audio_track_thread_running = false;
    bool compression_thread_running = false;
};

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

const int THIRTY_MINUTES = 60*30;
const int ONE_HOUR = 60*60;
const int TWO_HOURS = 120*60;

enum { HOUR = 3600, MINUTE = 60 };

struct Timestamp {
    int hours = 0 ;
    int minutes = 0;
    int seconds = 0;
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
    std::string final_output;
    FILE *process = popen(arg.c_str() , "r");
    if(process != NULL) {
        while(fgets(output_buffer, sizeof(output_buffer), process) != NULL) {
            if (flag == "verbose" || flag == "v") std::cout << output_buffer << std::endl;
            final_output += output_buffer;
        }
    }
    pclose(process);
    return final_output;
}


void determine_audio_tracks(std::unordered_map<std::string, Video_file>* movies, Program_status *program_status) {
    for (auto& movie : *movies) {
        program_status->audio_track_thread_running = true;
        std::string out = cmd_exec("ffprobe -v error -select_streams a -show_entries stream=codec_type -of default=nw=1:nk=1 \"" + movie.second.path + "\" | uniq -c", "");
        out.erase(remove_if(out.begin(), out.end(), isspace), out.end());
        movie.second.audio_track_count = stoi(out.substr(0,1));
        //std::cout << movie.second.audio_track_count << movie.second.video_title << std::endl;
        program_status->audio_track_progress++;
    }
}

/* 
1. convert movies
2. assign subs to movie struct
    
*/
void convert_subtitles(std::unordered_map<std::string, Video_file>* movies, Program_status *program_status, std::string folder_path) {
/*    for (auto& movie : *movies) {
        //cmd_exec("subtitleedit /convert \"" + movie.second.path + "\" subrip", "v");
    } */

    std::string subtitle_filename = "";
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (entry.path().extension() == ".srt") {
            subtitle_filename = entry.path().filename().string();
            int substr_lang_position_start = subtitle_filename.find_first_of(".") + 1;
            
            movies->at(subtitle_filename.substr(0, substr_lang_position_start) + "mkv").subtitle_langs.push_back(subtitle_filename.substr(substr_lang_position_start, 3));
        }
    }

}

inline Timestamp return_duration_seperated(int duration) {
    Timestamp video_duration;
    if ((duration / HOUR) >= 1) {
        video_duration.hours = duration / HOUR;
        duration = duration - HOUR * video_duration.hours;
    }

    if ((duration / MINUTE) >= 1) {
        video_duration.minutes = duration / MINUTE;
        duration = duration - MINUTE * video_duration.minutes;
    } 

    video_duration.seconds = duration;

/*     std::cout << "hours " << video_duration.hours << std::endl << "minutes " << video_duration.minutes << std::endl << "seconds " << video_duration.seconds << std::endl;
 */    return video_duration;
}

inline std::vector<Timestamp> create_timestamps(int duration, int screenshot_amount) {
    std::vector<Timestamp> timestamps;
    int duration_offset = duration / (screenshot_amount + 1);
    int timestamp = 0;
    for (int i = 0; i < screenshot_amount; i++) {
        timestamp += duration_offset;
        timestamps.push_back(return_duration_seperated(timestamp));
    }
    return timestamps;
}

void calculate_movie_aspect_ratios (std::unordered_map<std::string, Video_file>* movies, Program_status *program_status, std::string folder_path) {
    for (auto& movie : *movies) {
        std::string aspect_ratios = cmd_exec("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "");
        //output must be 
        //  (width)
        //  (height)
        movie.second.original_width = stoi(aspect_ratios.substr(0,4)); 
        movie.second.original_height = stoi(aspect_ratios.substr(5));

        const int duration = stoi(cmd_exec("ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "v"));

        std::cout << movie.second.original_width << "x" << movie.second.original_height << std::endl;
        //cmd_exec("ffmpeg -ss 00:12:30 -i \"" + movie.second.path + "\" -vframes 1 \"" + movie.second.video_title + ".png\"", "");

        std::vector<Timestamp> timestamps;
        if (duration < THIRTY_MINUTES) {
            timestamps = create_timestamps(duration, 2);
        } else if (duration < THIRTY_MINUTES) {
            timestamps = create_timestamps(duration, 3);
        } else if (duration < ONE_HOUR) {
            timestamps = create_timestamps(duration, 4);
        } else if (duration < TWO_HOURS) {
            timestamps = create_timestamps(duration, 5);
        } else if (duration > TWO_HOURS) {
            timestamps = create_timestamps(duration, 6);
        }

        std::cout << movie.second.video_title << std::endl;
        for(int i = 0; i < (int)timestamps.size(); i++) {
            Timestamp timestamp = timestamps.at(i); 
            std::string arg = std::format("ffmpeg -ss {0}:{1}:{2} -i \"{3}\" -vframes 1 \"{4}.{5}.png\"", timestamp.hours, timestamp.minutes, timestamp.seconds, movie.second.path, movie.second.video_title, i);
            std::cout << arg << std::endl;
            //cmd_exec(arg, "");

        }
    }
}

/*
void logger(Program_status *program_status) {
    std::string outstream = "";
    std::string backspace = "";
    while (program_status->compression_progress < program_status->movie_count) {
        outstream = program_status->audio_track_progress + " out of " + program_status->movie_count; 
        std::cout << outstream;
        for (int i = 0; i < (int)outstream.length(); i++) {
            backspace += "\b";
        }
        //std::cout << backspace;
        backspace = ""; 
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
*/


int main(int argc, char** argv)
{
    Program_status program_status;
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
            std::string temp_title = entry.path().filename().string();
            movie.video_title = temp_title.substr(0, temp_title.find_last_of("."));
            movies.insert({entry.path().filename(), movie});
            program_status.movie_count = movies.size();

        }
    }
    
/*     for (auto& movie : movies) {
        std::cout << movie.second.video_title << " " << movie.second.audio_track_count << std::endl;
        //cmd_exec("subtitleedit /convert \"" + movie + "\" subrip");
    } */
    //std::thread logger_thread (logger, &program_status);

/*     std::cout << "[INFO] Starting subtitle conversion thread" << std::endl;
 */    std::thread subtitle_thread (convert_subtitles, &movies, &program_status, path);

/*     std::cout << "[INFO] Starting audio track counting thread" << std::endl;
 */    std::thread audio_track_thread (determine_audio_tracks, &movies, &program_status);

/*     std::cout << "[INFO] Starting audio track counting thread" << std::endl;
*/     std::thread aspect_ratio_calculation_thread (calculate_movie_aspect_ratios, &movies, &program_status, path);


    audio_track_thread.join();
    program_status.audio_track_thread_running = false;
    subtitle_thread.join();
    program_status.subtitle_thread_running = false;
    aspect_ratio_calculation_thread.join();

    for (auto& movie : movies) {
        std::cout << movie.second.video_title << " " << movie.second.audio_track_count;
        for (auto& lang : movie.second.subtitle_langs) {
            std::cout << " " << lang;
        }
        std::cout << std::endl;
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

