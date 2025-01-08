#include <iostream>
#include <nlohmann/json.hpp>
#include <string_ops.h>


using json = nlohmann::json;

std::string list_files(std::string folder){
    json files = json::parse("[]");
    std::string fileContent = read_file("files.json");
    json jarray = json::parse(fileContent);
    for(std::string entry : jarray){
        if(is_in_string(entry, folder)){
        std::cout<<entry<<std::endl;
        files.push_back(entry);}
    }
    return files.dump(4);
}

int main(void){
    std::cout<<list_files("/hallo");
    return 0;
}