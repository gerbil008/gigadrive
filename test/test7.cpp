#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

void listFilesRecursive(const fs::path& folderPath, const fs::path& basePath, nlohmann::json& jsonArray) {
    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (fs::is_regular_file(entry.path())) {
                std::string relativePath = "/" + entry.path().string().substr(basePath.string().size() + 1);
                jsonArray.push_back(relativePath);
            } else if (fs::is_directory(entry.path())) {
                listFilesRecursive(entry.path(), basePath, jsonArray);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fehler: " << e.what() << std::endl;
    }
}

std::string listAllFilesAsJson(const std::string& folderPath) {
    nlohmann::json jsonArray = nlohmann::json::array();
    fs::path basePath = folderPath;
    while (basePath.has_parent_path() && basePath.filename() != "contents") {
        basePath = basePath.parent_path();
    }

    if (basePath.filename() != "contents") {
        std::cerr << "Fehler: 'contents' Verzeichnis nicht gefunden!\n";
        return "{}";
    }

    listFilesRecursive(folderPath, basePath, jsonArray);
    return jsonArray.dump(4);
}

int main() {
    std::string folder = "/home/gerbil/contents/Joni"; 
    std::cout << listAllFilesAsJson(folder) << std::endl;
    return 0;
}
