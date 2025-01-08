#include <string_ops.h>
#include <iostream>
#include <nlohmann/json.hpp>
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
    std::string returner = files.dump(4);
    return returner;
}

int main(void){
    std::cout<<list_files("/hallo")<<std::endl;
    return 0;
}