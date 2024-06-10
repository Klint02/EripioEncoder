struct EncodeLibInputs
{
    bool load_from_file = false;
    bool scan_only = false;
    bool verify = false;
    bool disable_video_encode = true;
    bool disable_audio_encode = true;
    bool disable_subtitle_conversion = true;
    bool recutter_enabled = false;
    int forced_sub_index = -1;
    int non_forced_sub_index = -1;

    std::string path = std::filesystem::current_path();
};

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
    int height[2] = {INT32_MAX, -1};
    
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

struct Timestamp_pair {
    std::string start;
    std::string end;
    int chapter_marked;
};

struct Recutter_video_file {
    std::string name;
    std::vector<Timestamp_pair> timestamps;
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



bool contains(int len, char** arr, std::string str1);

bool contains(int len, char** arr, std::string str1, std::string str2);

std::string cmd_exec(std::string arg, std::string flag);

void determine_audio_tracks(std::map<std::string, Video_file>* movies, Program_status *program_status, EncodeLibInputs inputs);

void convert_subtitles(std::map<std::string, Video_file>* movies, EncodeLibInputs inputs);

inline Timestamp return_duration_seperated(int duration);

inline std::vector<Timestamp> create_timestamps(int duration);

void calculate_movie_aspect_ratios(std::map<std::string, Video_file>* movies, EncodeLibInputs inputs);

std::string create_ffmpeg_argument(Video_file movie, std::string video_codec, std::string constant_rate_factor, EncodeLibInputs inputs);

void recutter(std::string path);

std::string create_tmp_directory();

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