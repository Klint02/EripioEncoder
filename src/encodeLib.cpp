#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <filesystem>
#include "encodeLib.hpp"
#include "raylib.h"

namespace fs = std::filesystem;

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
    std::cout << "Executing: \"" << arg << "\"" << std::endl;
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


void determine_audio_tracks(std::map<std::string, Video_file>* movies, Program_status *program_status, EncodeLibInputs inputs) {
    if (inputs.disable_audio_encode) {
        std::cout << "Audio enconding was disabled. Closing thread" << std::endl;
        return;
    }
    for (auto& movie : *movies) {
        program_status->audio_track_thread_running = true;

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


void convert_subtitles(std::map<std::string, Video_file>* movies, EncodeLibInputs inputs) {
    if (inputs.disable_subtitle_conversion) {
        std::cout << "Subtitle conversion was disabled. Closing thread" << std::endl;
        return;
    }
    for (auto& movie : *movies) {
        cmd_exec("subtitleedit /convert \"" + movie.second.path + "\" subrip", "v");
    }

    std::string subtitle_filename = "";
    for (const auto& entry : fs::directory_iterator(inputs.path)) {
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
void calculate_movie_aspect_ratios (std::map<std::string, Video_file>* movies, EncodeLibInputs inputs) {
    if (inputs.disable_video_encode) {
        std::cout << "Video encoding was disabled. Closing thread" << std::endl;
        return;
    }

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
            //Using tmp folder instead of movie folder as it would otherwise kill HDD activity
            std::string tmp_folderpath = create_tmp_directory();

            std::string arg = "ffmpeg -y -err_detect aggressive -fflags discardcorrupt -hide_banner -loglevel error -ss " + std::to_string(timestamp.hours) + ":" + std::to_string(timestamp.minutes) + ":" + std::to_string(timestamp.seconds) + " -i \"" + movie.second.path +"\" -vframes 1 \"" + tmp_folderpath + movie.second.video_title + "." + std::to_string(i) + ".png\"";
            //Use only for debug 
            //std::cout << arg << std::endl;
            cmd_exec(arg, "");
            std::string movie_frame_path = tmp_folderpath + movie.second.video_title + "." + std::to_string(i) + ".png";
            //std::string movie_frame_path = movie.second.path.substr(0, movie.second.path.find_last_of(".")) + "." + std::to_string(i) + ".png";  
            //TODO: only load image and run calculation if image exists
            Image frame = LoadImage(movie_frame_path.c_str());
            
            //Adjust to make it more or less discriminative of black pixels 
            //TODO: turn this into an argument for the program
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

std::string create_ffmpeg_argument(Video_file movie, std::string video_codec, std::string constant_rate_factor, EncodeLibInputs inputs) {
    std::string video_crop = "-filter:v \"crop=" + std::to_string(movie.width[1] - movie.width[0]) +  ":" + std::to_string(movie.height[1] - movie.height[0]) +  ":" + std::to_string(movie.width[0]) + ":" + std::to_string(movie.height[0]) + "\"";

    std::string ffmpeg_inputs_arg = "-i \"" + movie.path + "\" ";
    std::string ffmpeg_video_encode_arg = inputs.disable_video_encode ? "-codec:v copy " : "-codec:v " + video_codec + " " + constant_rate_factor + " " + video_crop + " ";
    std::string ffmpeg_audio_encode_arg = inputs.disable_audio_encode ? "-codec:a copy " : "";
    std::string ffmpeg_subtitle_metadata_arg = "";

    if (!inputs.disable_audio_encode) {
        for (u_int32_t i = 0; i < movie.audio_channel_count.size(); i++) {

            ffmpeg_audio_encode_arg += "-c:a:" + std::to_string(i) + " eac3 ";
            if (movie.audio_channel_count[i] > 6) ffmpeg_audio_encode_arg += "-ac:a:" + std::to_string(i) + " 6 ";
        }
    }

    if (!inputs.disable_subtitle_conversion) {
        for (u_int32_t i = 0; i < movie.subtitle_langs.size(); i++) {
            //Skips the addition of another subtitle track if there is no language
            //Because it would otherwise make ffmpeg look after MOVIETITLE..mkv
            if (movie.subtitle_langs[i].length() > 0) {
                ffmpeg_subtitle_metadata_arg += " -map " + std::to_string(i+1) + ":s -metadata:s:s:" + std::to_string(i) + " language=" + movie.subtitle_langs[i];
                ffmpeg_inputs_arg += "-i \"" + inputs.path + "/" +  movie.video_title + "." + movie.subtitle_langs[i] + ".srt\" ";
            }

        }

        if (ffmpeg_subtitle_metadata_arg.size() > 0) { 
            ffmpeg_subtitle_metadata_arg += " -c:s copy";
            //demapping old subs embedded in file
            ffmpeg_subtitle_metadata_arg += " -map -0:s ";

        }
    }
    if (inputs.disable_video_encode + inputs.disable_video_encode + inputs.disable_subtitle_conversion == 3) {
        
        if (inputs.forced_sub_index > 0) {
            return "mkvpropedit \"" + movie.path + "\" --edit info --set \"title=" + movie.video_title + "\"" + " --edit track:s" + std::to_string(inputs.forced_sub_index) + " --set flag-forced=1";
        } 
        
        if (inputs.non_forced_sub_index > 0) {
            return "mkvpropedit \"" + movie.path + "\" --edit info --set \"title=" + movie.video_title + "\"" + " --edit track:s" + std::to_string(inputs.non_forced_sub_index) + " --set flag-forced=0";
        }

        if (inputs.non_forced_sub_index + inputs.forced_sub_index <= -1) {
            return "mkvpropedit \"" + movie.path + "\" --edit info --set \"title=" + movie.video_title + "\"";
        }
    }
    return "ffmpeg " + ffmpeg_inputs_arg +  " -map 0 " + ffmpeg_video_encode_arg + ffmpeg_audio_encode_arg + ffmpeg_subtitle_metadata_arg + " " + " -metadata title=\"" + movie.video_title + "\" \"" + inputs.path + "/0encoded/" + movie.video_title + ".mkv\"";
}

void recutter(std::string path) {
    std::string tmp_folder_path = create_tmp_directory();
    std::vector<Recutter_video_file> video_files;
    std::string line;
    std::ifstream config_file(path + "/recutter.txt");
    if (config_file.is_open()) {
        while (getline(config_file, line)) {
            //std::cout << line << std::endl;
            switch (line[0])
            {
            case '#':
                video_files.push_back(Recutter_video_file());
                video_files.back().name = line.substr(2);
                break;
            
            case '%':
                if (video_files.size() == 0) 
                {
                    std::cout << "Recutter tried to insert timestamp, but no file was given" << std::endl; 
                    return;
                } 
                
                video_files.back().timestamps.push_back(Timestamp_pair());
                video_files.back().timestamps.back().start = line.substr(2,8);
                video_files.back().timestamps.back().end = line.substr(11,8);
                video_files.back().timestamps.back().chapter_marked = stoi(line.substr(20,1));
                break;
            default:
                break;
            }
        }
    }
    for (auto& video_file : video_files) {
        std::cout << video_file.name << std::endl;
        std::string clip_string = "";
        std::string chapter_metadata_string = ";FFMETADATA1 \n\n";
        u_int8_t chapter_count = 1;
        u_int32_t chapter_seconds_timestamp = 0;
        for (u_int8_t i = 0; i < video_file.timestamps.size(); i++) {
            cmd_exec("ffmpeg -i \"" + video_file.name + "\" -map 0 -c copy -ss " + video_file.timestamps[i].start + " -to " + video_file.timestamps[i].end + " -y \"" + tmp_folder_path +  "clip" + std::to_string(i) + ".mkv\"", "v");
            clip_string += "file '" + tmp_folder_path + "clip" + std::to_string(i) + ".mkv'\n";
            int clip_length = std::stoi(video_file.timestamps[i].end.substr(0, 2)) * 3600 + std::stoi(video_file.timestamps[i].end.substr(3, 2)) * 60 + std::stoi(video_file.timestamps[i].end.substr(6, 2));
            clip_length -= std::stoi(video_file.timestamps[i].start.substr(0, 2)) * 3600 + std::stoi(video_file.timestamps[i].start.substr(3, 2)) * 60 + std::stoi(video_file.timestamps[i].start.substr(6, 2));
            if (video_file.timestamps[i].chapter_marked) {
                chapter_metadata_string += "[CHAPTER]\nTIMEBASE=1/1000\nSTART=" + std::to_string(chapter_seconds_timestamp*1000) +"\nEND=" +std::to_string(chapter_seconds_timestamp*1000) + "\ntitle=Chapter " + std::to_string(chapter_count) + "\n\n";
                chapter_count++;
            }
            chapter_seconds_timestamp += clip_length;
        }
        std::ofstream clips(tmp_folder_path + "clips.txt", std::ios::trunc);
        clips << clip_string;
        clips.close();
        std::ofstream metadata(tmp_folder_path + "metadata.txt", std::ios::trunc);
        metadata << chapter_metadata_string;
        metadata.close();
        std::cout << "ffmpeg -f concat -safe 0 -i \"" + tmp_folder_path + "clips.txt\" " + "-i \"" + tmp_folder_path + "metadata.txt\" -map_metadata 1 -map 0 -c copy output.mkv" << std::endl;
        //cmd_exec("ffmpeg -i \"" + clip_string + "\" -c copy " + chapter_metadata_string + " output.mkv", "v");
    }
}
std::string create_tmp_directory () {
    std::string tmp_folderpath = fs::temp_directory_path();
    tmp_folderpath = tmp_folderpath + "/eripio/";
    fs::directory_entry tmpfolder{ tmp_folderpath };
    if (!tmpfolder.exists()) {
        fs::create_directory(tmp_folderpath);
    }
    return tmp_folderpath;
}