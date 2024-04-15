#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <vector>
#include <thread>
#include "encodeLib.hpp"


namespace fs = std::filesystem;



int main(int argc, char** argv)
{
    EncodeLibInputs inputs;
    Program_status program_status;
    std::map<std::string, Video_file> movies;

    if (contains(argc, argv, "-h", "--help")) {
        std::cout 
        << "Welcome to the encoder \n" 
        << "    -h  or --help \t This message \n\n"
        << "    -p  or --path \t Specify directory for encoding \n"
        << "    -l  or --load \t Load config from a previous run \n"
        << "    -s  or --scan \t Create config file without running compression \n"
        << "    -c  or --check\t Verify encoded videos \n"
        << "    \n  Keep in mind that -s can only be used by itself or with -l AND -c \n\n"
        << "  Use the following to bypass part of the encoder \n"
        << "    -r  or --remux\t Sets the title tag in the mkv \n"
        << "    -da \t \t Disables the audio encoder \n"
        << "    -dv \t \t Disables the video encoder \n"
        << "    -ds \t \t Disables the subtitle converter \n"
        << "    -da -dv -ds is the same as -r og --remux \n\n\n"
        << "    -fs \t \t Changes the provided subtitle track index to forced \n"
        << "    -dfs \t \t Changes the provided subtitle track index to not be forced \n"
        << std::endl;
        return 0;
    } else {
        if  (contains(argc, argv, "-p", "--path")) {
            for (int i = 0; i < argc; i++) {
                if (((std::string)argv[i] == "-p" || (std::string)argv[i] == "--path")) {
                    if (argc - 1 != i) {
                        inputs.path = argv[i + 1];
                        if (inputs.path[0] == '~') {
                            inputs.path = std::getenv("HOME") + inputs.path.substr(1);
                        }
                    } else {
                        std::cout << "No path given" << std::endl;
                        return 0;
                    }
                }
            }
        }

        //no need to use ifs since they should be set to true if they are found
        inputs.load_from_file = contains(argc, argv, "-l", "--load");   
        inputs.scan_only = contains(argc, argv, "-s", "--scan");
        inputs.verify = contains(argc, argv, "-c", "--check");
        inputs.disable_video_encode = contains(argc, argv, "-r", "--remux") || contains(argc, argv, "-dv");
        inputs.disable_audio_encode = contains(argc, argv, "-r", "--remux") || contains(argc, argv, "-da");
        inputs.disable_subtitle_conversion = contains(argc, argv, "-r", "--remux") || contains(argc, argv, "-ds");

        if (!inputs.verify && inputs.scan_only && inputs.load_from_file) {
            std::cout << "[ERROR] Cannot use scan and load together \n" 
                      << "Please consult the help page by using -h or --help \n" 
                      << std::endl; 
            return 0;
        }

        if (contains(argc, argv, "-fs")) {
            for (int i = 0; i < argc; i++) {
                if ((std::string)argv[i] == "-fs" && i != argc -1) {
                    try
                    {
                        inputs.forced_sub_index = std::stoi(argv[i+1]);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << std::endl;
                    }
                }
            }
        }

        if (contains(argc, argv, "-dfs")) {
            for (int i = 0; i < argc; i++) {
                if ((std::string)argv[i] == "-dfs" && i != argc -1) {
                    try
                    {
                        inputs.forced_sub_index = std::stoi(argv[i+1]);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << std::endl;
                    }
                }
            }
        }
/*
        if (verify && scan_only) {
            std::cout << "[ERROR] Cannot use scan and verify together \n" 
                      << "Please consult the help page by using -h or --help \n" 
                      << std::endl; 
            return 0;
        }
*/
    }

    if (!inputs.load_from_file) {
        std::cout << "[INFO] Determining audio tracks for movies" << std::endl;
        for (const auto& entry : fs::directory_iterator(inputs.path)) 
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
        std::thread convert_subtitles_thread (convert_subtitles, &movies, inputs);

        std::cout << "[INFO] Starting audio track counting thread" << std::endl;
        std::thread audio_track_thread (determine_audio_tracks, &movies, &program_status, inputs);

        std::cout << "[INFO] Starting aspect ratio calculation thread" << std::endl;
        std::thread aspect_ratio_calculation_thread (calculate_movie_aspect_ratios, &movies, inputs);
    
        convert_subtitles_thread.join();
        audio_track_thread.join();
        aspect_ratio_calculation_thread.join();

        std::cout << inputs.path << std::endl;
        std::ofstream config_file;
        //open with truncation so old movie config gets removed
        config_file.open(inputs.path + "/movies.txt", std::ios::trunc);
        


        for (auto& movie : movies) {
            config_file << movie.second.to_string() << std::endl;
        } 

        config_file.close();
    } else {
        std::string line;
        std::ifstream config_file (inputs.path + "/movies.txt");
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
                                //I don't remember why original height is 18000
                                movies.at(movie_key).original_height = 18000;
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
    
    if (!inputs.scan_only) {
        fs::directory_entry entry{inputs.path + "/0encoded"};
        if (!entry.exists()) {
            fs::create_directory(inputs.path + "/0encoded");
        }
        
        for (auto& movie : movies) {
            //TODO: Fetch argument two and three from argv
            //TODO: Create logging file for user to see if ffmpeg crashed
            cmd_exec(create_ffmpeg_argument(movie.second, "libx265", "-crf 21", inputs), "-v");
        } 
    }

    if (inputs.verify) {
        //TODO: Should also check if any of the files are missing. But verification would still fail right now without that check 
        for (auto& movie : movies) {
            bool movie_integrity_match = true;
            fs::directory_entry entry{inputs.path + "/0encoded/" + movie.second.video_title + ".mkv"};
            if (entry.exists()) {

            
                std::string original_duration = cmd_exec("ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 \"" + movie.second.path + "\"", "");
                std::string encoded_duration = cmd_exec("ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 \"" + inputs.path + "/0encoded/" + movie.second.video_title + ".mkv" + "\"", "");
                
                if (encoded_duration.find("N/A") != std::string::npos) {
                    movie_integrity_match = false;
                    std::cout << "Duration of video was N/A, ffmpeg might have crashed while encoding this video" << std::endl;
                }

                //short circuiting should prevent exceptions with stoi
                if (movie_integrity_match && !((stol(original_duration) - stol(encoded_duration)) < 2 && (stol(original_duration) - stol(encoded_duration)) > -2)) {
                    movie_integrity_match = false;
                    std::cout << "Video durations did not match \n" 
                              << stol(original_duration) << "\n"
                              << stol(encoded_duration) << std::endl; 
                }

            } else {
                movie_integrity_match = false;
            }

            if (!movie_integrity_match) {
                //TODO: Add logging for this
                std::cout << movie.second.video_title << " failed verification. Please check yourself or just reencode original video \n" << std::endl;
            }
        }
    }

    return 0;
}

