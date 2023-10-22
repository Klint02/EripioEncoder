#include <cmath>
#include <filesystem>
namespace fs = std::filesystem;

class Encoder 
{
    public:
        enum resolution { FHD_WIDTH = 1920, FHD_HEIGHT = 1080, UHD_WIDTH = 3840, UHD_HEIGHT = 2160 };

        struct Video_file {
            std::filesystem::directory_entry path;
            std::string video_title;

            int subtitle_count;

            int original_width;

            int width;
            int height;
            
        };

        Video_file video;

        void find_resolution (std::filesystem::directory_entry entry) 
        {
            auto batch_folder_name = entry.path().filename().string();
            if (entry.is_directory() && batch_folder_name.find(":") != std::string::npos) 
            {
                int delimiter = batch_folder_name.find(":");
                float width_ratio = stof(batch_folder_name.substr(0, delimiter));
                if (width_ratio > (float)1.78) 
                { //TODO: Looks shit, but works. Make it look better
                    if (video.original_width == UHD_WIDTH) {
                        video.height = ceil(UHD_WIDTH / width_ratio);
                        video.width = UHD_WIDTH;
                    } else {
                        video.height = ceil(FHD_WIDTH / width_ratio);
                        video.width = FHD_WIDTH;
                    }
                } else if (width_ratio < (float)1.78) {
                    if (video.original_width == UHD_WIDTH) {
                        video.width = ceil(UHD_HEIGHT * width_ratio);
                        video.height = UHD_HEIGHT;
                    } else {
                        video.width = ceil(FHD_HEIGHT * width_ratio);
                        video.height = FHD_HEIGHT;
                    }
                } else {
                    if (video.original_width == UHD_WIDTH) {
                        video.width = UHD_WIDTH;
                        video.height = UHD_HEIGHT;
                    } else {
                        video.width = FHD_WIDTH;
                        video.height = FHD_HEIGHT;
                    }
                }
                if (video.width % 2 > 0) {
                    video.width++;
                }
                if (video.height % 2 > 0) {
                    video.height++;
                }
            }
        }

        void set_video_metadata(std::filesystem::directory_entry entry) {
            entry.path().filename().string();
        }

        void run(std::filesystem::directory_entry entry) {
            for (const auto& file : fs::directory_iterator(entry.path())) 
            {
                char output_buffer [1024];

                std::string arg = "ffprobe -v error -select_streams v:0 -show_entries stream=width -of default=nw=1:nk=1 \"" + file.path().string() + "\""; 
                FILE *process_ffprobe = popen(arg.c_str() , "r");
                if(process_ffprobe != NULL) {
	                while(fgets(output_buffer, sizeof(output_buffer), process_ffprobe) != NULL) {
                        std::cout << output_buffer << std::endl;
                    }
                }
                pclose(process_ffprobe);
                
                video.original_width = std::stoi(output_buffer);

                find_resolution(entry);

                arg = "subtitleedit /convert \"" + file.path().string() + "\" subrip";
                std::cout << arg << std::endl;
                FILE *subtitleedit = popen(arg.c_str() , "r");
                if(subtitleedit != NULL) {
	                while(fgets(output_buffer, sizeof(output_buffer), subtitleedit) != NULL) {
                        std::cout << output_buffer << std::endl;

                    }
                }
                pclose(subtitleedit);

                arg = "ffmpeg -i \"" + file.path().string() + "\"  -map 0 -codec:v copy -codec:a copy -map -0:s test.mkv"; 
                FILE *ffmpeg = popen(arg.c_str() , "r");
                if(ffmpeg != NULL) {
	                while(fgets(output_buffer, sizeof(output_buffer), ffmpeg) != NULL) {
                        std::cout << output_buffer << std::endl;

                    }
                }
                pclose(ffmpeg);

                std::cout << output_buffer << std::endl;



            }
            

/*
            if (current_path == "") {
                std::cerr << "No path was set" << std::endl;
                throw(0);
            }
*/

            //std::cout << "FHD_video_width " << FHD_video_width << " FHD_video_height " << FHD_video_height << std::endl;

        }
};