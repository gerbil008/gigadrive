#include <iostream>
#include <filesystem>
#include <string>
#include <string_ops.h>

std::string path = "/home/gerbil/contents/";

using namespace std;
namespace fs = std::filesystem;

int make_file(std::string filename){
    if(startswith(filename, '/')){
        filename = strip_first(filename);
    }
    if(!fs::exists(path+fs::path(filename).parent_path().string())){
        std::cout<<fs::path(filename).parent_path().string()<<std::endl;
        std::cout<<path+fs::path(filename).parent_path().string();
        fs::create_directories(path+fs::path(filename).parent_path().string());
    }
    //fs::create_directories(path+filename);
    write_file(path+filename, "");
    //write_json("files.txt", filename);
    return 0;
}

int main(void){
    make_file("/hallo/hallo.txt");
    return 0;
}