#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
class Encoder {
    private:
        int test;
    public: 

        void assign_test(int value) {
            this->test = value;
        }
        void print_test() {
            
            std::cout << this->test << std::endl;
        }

        void static hmm(){
            std::cout << "wow" <<std::endl;
        }
};


int main()
{
    
    std::string current_path = fs::current_path();
    
    std::cout << current_path << std::endl;

}

