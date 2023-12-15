#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <vector>
#include <thread>
#include <algorithm>
#include "raylib.h"


namespace fs = std::filesystem;

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

    std::vector<u_int16_t> audio_channel_count;

    int original_height;
    int original_width;

    int width[2] = {INT32_MAX, -1};
    int height[2] = {INT32_MAX, -1};;
    
    std::string to_string() 
    {
        std::string sub_langs = "";
        std::string audio_tracks = "";
        std::string delimiter = "";
        
        for (auto& lang : subtitle_langs) {
            sub_langs += delimiter + lang;
            delimiter = ",";
        }

        delimiter = "";
        for (auto& track : audio_channel_count) {
            audio_tracks += delimiter + std::to_string(track);
            delimiter = ",";
        }

        return path + "|" + video_title + "|" + sub_langs + "|" + audio_tracks + "|" + std::to_string(original_height) + "|" + std::to_string(original_width) + "|" + std::to_string(width[0]) + "," + std::to_string(width[1]) + "|" + std::to_string(height[0]) + "," + std::to_string(height[1]);
    }
};

enum { LETTERBOX, PILLARBOX , FULLBOX, UNDEFINED = -1 };

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


void determine_audio_tracks(std::map<std::string, Video_file>* movies, Program_status *program_status) {
    for (auto& movie : *movies) {
        program_status->audio_track_thread_running = true;
        /*
        std::string out = cmd_exec("ffprobe -v error -select_streams a -show_entries stream=codec_type -of default=nw=1:nk=1 \"" + movie.second.path + "\" | uniq -c", "");
        out.erase(remove_if(out.begin(), out.end(), isspace), out.end());
        movie.second.audio_track_count = stoi(out.substr(0,1));
*/
        std::string out = cmd_exec("ffprobe -v error -select_streams a -show_entries stream=channels -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "");
        std::cout << movie.second.video_title << std::endl << out << std::endl;
        int substring_start = 0;
        for(u_int32_t i = 0; i < out.length(); i++) {
            if (out[i] == '\n') {
                //std::cout << out.substr(substring_start, i - substring_start) << std::endl;
                movie.second.audio_channel_count.push_back(stoi(out.substr(substring_start, i - substring_start)));
                //newline_pos.push_back(i);
                substring_start = i;
            }
        }


        //std::cout << movie.second.audio_track_count << movie.second.video_title << std::endl;
        program_status->audio_track_progress++;
    }
}


void convert_subtitles(std::map<std::string, Video_file>* movies, Program_status *program_status, std::string folder_path) {
    for (auto& movie : *movies) {
        cmd_exec("subtitleedit /convert \"" + movie.second.path + "\" subrip", "v");
    }

    std::string subtitle_filename = "";
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (entry.path().extension() == ".srt") {
            subtitle_filename = entry.path().filename().string();
            cmd_exec("sed -i 's/|/I/gI' \"" + entry.path().string() + "\"", "");
            cmd_exec("sed -i 's/-1/-I/gI' \"" + entry.path().string() + "\"", "");
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

inline std::vector<Timestamp> create_timestamps(int duration) {
    std::vector<Timestamp> timestamps;

    //Remove as many seconds of credits as possible while preserving usable movie frames
    if (duration > (HOUR * 2)) {
        duration = duration - MINUTE * 45;
    } else if (duration > HOUR) {
        duration = duration - MINUTE * 30;
    } 
    for (int timestamp = MINUTE * 5; timestamp < duration; timestamp += MINUTE * 5) {
        timestamps.push_back(return_duration_seperated(timestamp));
    }
    return timestamps;
}

//TODO: Find a way to sort the corrupted frames from the good (lego movie for testing)
void calculate_movie_aspect_ratios (std::map<std::string, Video_file>* movies, Program_status *program_status, std::string folder_path) {
    for (auto& movie : *movies) {
        std::string aspect_ratios = cmd_exec("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "");
        //output must be 
        //  (width)
        //  (height)
        movie.second.original_width = stoi(aspect_ratios.substr(0,4)); 
        movie.second.original_height = stoi(aspect_ratios.substr(5));

        const int duration = stoi(cmd_exec("ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "v"));

        std::cout << movie.second.original_width << "x" << movie.second.original_height << std::endl;

        std::vector<Timestamp> timestamps = create_timestamps(duration);

        std::cout << movie.second.video_title << std::endl;
        //int aspect_ratio_type = -1;

        for(int i = 0; i < (int)timestamps.size(); i++) {
            Timestamp timestamp = timestamps.at(i); 
            std::string arg = "ffmpeg -y -err_detect aggressive -fflags discardcorrupt -hide_banner -loglevel error -ss " + std::to_string(timestamp.hours) + ":" + std::to_string(timestamp.minutes) + ":" + std::to_string(timestamp.seconds) + " -i \"" + movie.second.path +"\" -vframes 1 \"" + movie.second.video_title + "." + std::to_string(i) + ".png\"";
            //Use only for debug 
            //std::cout << arg << std::endl;
            cmd_exec(arg, "");
            std::string movie_frame_path = movie.second.path.substr(0, movie.second.path.find_last_of(".")) + "." + std::to_string(i) + ".png";  
            Image frame = LoadImage(movie_frame_path.c_str());
            
            //Adjust to make it more or less discriminative of black pixels 
            int over_correction = 2;

            //TODO: make this smaller?
            int p_y = 0;
            int accumulator_pixel = 0;
            //Why 10?
            //because 8 controlpixels are enough I think
            int pixel_offset = movie.second.original_width / 10;
            int movie_middle = movie.second.original_height / 2;

            while (p_y < movie_middle && accumulator_pixel < 1) {
                for (int i = 2; i < 10; i++) {
                    Color pixel = GetImageColor(frame, pixel_offset*i, p_y);
                    accumulator_pixel += ((int)pixel.r >> over_correction) + ((int)pixel.g >> over_correction) + ((int)pixel.b >> over_correction);
                }
                p_y++;
            }
            if (p_y < movie.second.height[0] && (p_y != 1 && p_y + 2 != movie.second.original_height)) movie.second.height[0] = p_y; 

            p_y = movie.second.original_height -1;
            accumulator_pixel = 0;
            while (p_y > movie_middle && accumulator_pixel < 1) {
                for (int i = 2; i < 10; i++) {
                    Color pixel = GetImageColor(frame, pixel_offset*i, p_y);
                    accumulator_pixel += ((int)pixel.r >> over_correction) + ((int)pixel.g >> over_correction) + ((int)pixel.b >> over_correction);
                }
                p_y--;
            }
            if (p_y > movie.second.height[1] && (p_y != 1 && p_y + 2 != movie.second.original_height)) movie.second.height[1] = p_y; 

            int p_x = 0;
            accumulator_pixel = 0;
            //Why 10?
            //because 8 controlpoints are enough I think
            pixel_offset = movie.second.original_height / 10;
            movie_middle = movie.second.original_width / 2;

            while (p_x < movie_middle && accumulator_pixel < 1) {
                for (int i = 2; i < 10; i++) {
                    Color pixel = GetImageColor(frame, p_x, pixel_offset*i);
                    accumulator_pixel += ((int)pixel.r >> over_correction) + ((int)pixel.g >> over_correction) + ((int)pixel.b >> over_correction);
                }
                p_x++;
            }
            if (p_x < movie.second.width[0] && (p_x != 1 && p_x + 2 != movie.second.original_width)) movie.second.width[0] = p_x; 

            p_x = movie.second.original_width -1;
            accumulator_pixel = 0;
            while (p_x > movie_middle && accumulator_pixel < 1) {
                for (int i = 2; i < 10; i++) {
                    Color pixel = GetImageColor(frame, p_x, pixel_offset*i);
                    
                    accumulator_pixel += ((int)pixel.r >> over_correction) + ((int)pixel.g >> over_correction) + ((int)pixel.b >> over_correction);
                }
                p_x--;
            }
            if (p_x > movie.second.width[1] && (p_x != 1 && p_x + 2 != movie.second.original_width)) movie.second.width[1] = p_x; 
            
            //if black bars were not found at all, the remaining width and heights are set. 
            //must be done per frame and not at the end, as movies like the conjuring has singular scenes where there are black bars all around
            if (movie.second.width[0] == INT32_MAX) movie.second.width[0] = 0;
            if (movie.second.width[1] == -1) movie.second.width[1] = movie.second.original_width;
            if (movie.second.height[0] == INT32_MAX) movie.second.height[0] = 0;
            if (movie.second.height[1] == -1) movie.second.height[1] = movie.second.original_height;
            
            std::cout << "width calculation " << movie.second.width[0] << " " << movie.second.width[1] << std::endl;
            std::cout << "height calculation " << movie.second.height[0] << " " << movie.second.height[1] << std::endl;
            //Discards the current frame from memory to prevent memory leaks
            UnloadImage(frame);
        }




        std::cout << "width calculation " << movie.second.width[0] << " " << movie.second.width[1] << std::endl;
        std::cout << "height calculation " << movie.second.height[0] << " " << movie.second.height[1] << std::endl;
    }
}

inline std::string create_ffmpeg_argument(Video_file movie, std::string video_codec, std::string constant_rate_factor, std::string path) {
    
    std::string audio_args = "";
    for (u_int32_t i = 0; i < movie.audio_channel_count.size(); i++) {

        audio_args += "-c:a:" + std::to_string(i) + " eac3 ";
        if (movie.audio_channel_count[i] > 6) audio_args += "-ac:a:" + std::to_string(i) + " 6 ";
    }
    std::string video_crop_args = "-filter:v \"crop=" + std::to_string(movie.width[1] - movie.width[0]) +  ":" + std::to_string(movie.height[1] - movie.height[0]) +  ":" + std::to_string(movie.width[0]) + ":" + std::to_string(movie.height[0]) + "\"";
    
    std::string subtitle_metadata_args = "";
    std::string subtitle_input_args = "";
    std::string subtile_demap = "";

    for (u_int32_t i = 0; i < movie.subtitle_langs.size(); i++) {
        subtitle_metadata_args += " -map " + std::to_string(i+1) + ":s -metadata:s:s:" + std::to_string(i) + " language=" + movie.subtitle_langs[i];
        subtitle_input_args += "-i \"" + path + "/" +  movie.video_title + "." + movie.subtitle_langs[i] + ".srt\" ";
    }

    if (subtitle_metadata_args.size() > 0) { 
        subtitle_metadata_args += " -c:s copy";
        subtile_demap = " -map -0:s ";
    }

    
    return "ffmpeg -i \"" + movie.path + "\" " + subtitle_input_args +  " -map 0 -codec:v " + video_codec + " " + constant_rate_factor + " " + video_crop_args + " " + audio_args + subtile_demap + subtitle_metadata_args + " " + " -metadata title=\"" + movie.video_title + "\" \"" + path + "/0encoded/" + movie.video_title + ".mkv\"";
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
    bool load_from_file = false;
    Program_status program_status;
    std::map<std::string, Video_file> movies;

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

        if (contains(argc, argv, "-l", "--load")) {
            load_from_file = true;   
        }

    }

    if (!load_from_file) {
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
        
        //TODO: Create logging screen with ncurses
        //std::thread logger_thread (logger, &program_status);
        std::cout << "[INFO] Starting subtitle conversion" << std::endl;
        std::thread convert_subtitles_thread (convert_subtitles, &movies, &program_status, path);

        std::cout << "[INFO] Starting audio track counting thread" << std::endl;
        std::thread audio_track_thread (determine_audio_tracks, &movies, &program_status);

        std::cout << "[INFO] Starting aspect ratio calculation thread" << std::endl;
        std::thread aspect_ratio_calculation_thread (calculate_movie_aspect_ratios, &movies, &program_status, path);
    
        convert_subtitles_thread.join();
        audio_track_thread.join();
        aspect_ratio_calculation_thread.join();

        std::cout << path << std::endl;
        std::ofstream config_file;
        //open with truncation so old movie config gets removed
        config_file.open(path + "/movies.txt", std::ios::trunc);
        


        for (auto& movie : movies) {
            config_file << movie.second.to_string() << std::endl;
        } 

        config_file.close();
    } else {
        std::string line;
        std::ifstream config_file (path + "/movies.txt");
        if (config_file.is_open()) {
            while (getline(config_file, line)) {
                std::string movie_key = "";
                Video_file movie;

                int substring_start = 0;
                int subsubstring_start = 0;

                int field_index = 0;

                for(u_int32_t i = 0; i < line.length(); i++) {
                    

                    if (line[i] == '|' || i == line.length() - 1) {
                        int delimiter;
                        std::string struct_field = "";
                        //Stupid fix because height was missing a char
                        if (i == line.length() - 1) {
                            struct_field = line.substr(substring_start);
                        } else {
                            struct_field = line.substr(substring_start, i - substring_start);
                            substring_start = i + 1;

                        }

                        switch (field_index) {
                            case 0:
                            std::cout << field_index << std::endl;
                                movie_key = struct_field;
                                movie.path = struct_field;
                                movies.insert({struct_field, movie});

                                break;
                            case 1:
                                movies.at(movie_key).video_title = struct_field;
                                break;
                            case 2:
                                subsubstring_start = 0;
                                if (std::string::npos != struct_field.find(',')) {
                                    for (u_int32_t j = 0; j < struct_field.length(); j++) {
                                        if (struct_field[j] == ',') {
                                            movies.at(movie_key).subtitle_langs.push_back(struct_field.substr(subsubstring_start, j - subsubstring_start));
                                            subsubstring_start = j;
                                        }
                                        if (j == struct_field.length() - 1) {
                                            movies.at(movie_key).subtitle_langs.push_back(struct_field.substr(subsubstring_start + 1));
                                        }
                                    }
                                } else {
                                    movies.at(movie_key).subtitle_langs.push_back(struct_field);
                                }
                                break;
                            case 3:
                                subsubstring_start = 0;
                                if (std::string::npos != struct_field.find(',')) {
                                    for (u_int32_t j = 0; j < struct_field.length(); j++) {
                                        if (struct_field[j] == ',') {
                                            movies.at(movie_key).audio_channel_count.push_back(stoi(struct_field.substr(subsubstring_start, j - subsubstring_start)));
                                            subsubstring_start = j;
                                        }
                                        if (j == struct_field.length() - 1) {
                                            movies.at(movie_key).audio_channel_count.push_back(stoi(struct_field.substr(subsubstring_start + 1)));
                                        }
                                    }
                                } else {
                                    movies.at(movie_key).audio_channel_count.push_back(stoi(struct_field));
                                }
                                break;
                            case 4:
                                movies.at(movie_key).original_height = 18000;
                                std::cout << stoi(struct_field) << std::endl;
                                break;
                            case 5: 
                                movies.at(movie_key).original_width = stoi(struct_field);
                                break;
                            case 6: 
                                delimiter = struct_field.find_first_of(',');
                                movies.at(movie_key).width[0] = stoi(struct_field.substr(0,delimiter));
                                movies.at(movie_key).width[1] = stoi(struct_field.substr(delimiter + 1));
                                break;
                            case 7:
                                delimiter = struct_field.find_first_of(',');
                                movies.at(movie_key).height[0] = stoi(struct_field.substr(0,delimiter));
                                movies.at(movie_key).height[1] = stoi(struct_field.substr(delimiter + 1));
                                break;
                        }

                        field_index++;
                    }
                }
            }
        }
    }
    

    fs::directory_entry entry{path + "/0encoded"};
    if (!entry.exists()) {
        fs::create_directory(path + "/0encoded");
    }
    
    for (auto& movie : movies) {
        std::cout << create_ffmpeg_argument(movie.second, "libx265", "-crf 21", path) << std::endl;
        //cmd_exec(create_ffmpeg_argument(movie.second, "libx265", "-crf 21", path), "-v");
    } 
    return 0;
}

