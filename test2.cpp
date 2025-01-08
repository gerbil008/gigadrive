#include <iostream>
#include <nlohmann/json.hpp>
#include <string_ops.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string path = "/home/gerbil/contents";

std::string trim_ex(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

void append_to_json(const std::string& filename, const std::string& newEntry) {
    try {
        std::string fileContent = read_file(filename);
        json database;

        if (fileContent.empty()) {
            database = json::array();
        } else {
            try {
                database = json::parse(fileContent);
            } catch (const json::parse_error& e) {
                throw std::runtime_error("Ungültige JSON-Datei: " + std::string(e.what()));
            }
        }

        if (!database.is_array()) {
            throw std::runtime_error("Die JSON-Datenbank ist kein Array!");
        }

        std::string trimmedEntry = trim_ex(newEntry);

        try {
            json newJsonEntry = trimmedEntry; 
            database.push_back(newJsonEntry);
        } catch (const std::exception& e) {
            throw std::runtime_error("Ungültiger JSON-String für den neuen Eintrag: " + std::string(e.what()));
        }

        write_file(filename, database.dump(4));
    } catch (const std::exception& e) {
        throw std::runtime_error("Fehler beim Anhängen an die JSON-Datenbank: " + std::string(e.what()));
    }
}

void log(std::string msg){
    std::cout<<msg<<std::endl;
}

int make_file(std::string filename){
    if(!fs::exists(path+fs::path(filename).parent_path().string())){
        std::cout<<fs::path(filename).parent_path().string()<<std::endl;
        std::cout<<path+fs::path(filename).parent_path().string();
        fs::create_directories(path+fs::path(filename).parent_path().string());
    }
    write_file(path+filename, "");
    append_to_json("files.json", filename);
    return 0;
}

int check_if_make_fille(std::string filename){
    json jarray = read_file("files.json");
    if (std::find(jarray.begin(), jarray.end(), filename) == jarray.end()) {
        std::cout<<"Found";
    }
    else{
        make_file(trim_ex(filename));
        log("Maked file");
    }
    return 0;

}

int main(void){
    check_if_make_fille("/moin.txt");
    return 0;
}