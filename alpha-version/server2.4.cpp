#include "fileserver4.hpp"

int identnum = 100;
std::map<SSL*, std::string> filenames;
std::set<SSL*> m_connections;

int get_storage_free(const std::string& path) {
    struct statvfs buffer;
    statvfs(path.c_str(), &buffer);
    unsigned long long total_space = buffer.f_blocks * buffer.f_frsize;
    return static_cast<int>(total_space / (1024 * 1024 * 1024));
}

int get_storage_full(const std::string& path) {
    struct statvfs buffer;
    statvfs(path.c_str(), &buffer);
    unsigned long long total_space = buffer.f_blocks * buffer.f_frsize;
    unsigned long long free_space = buffer.f_bfree * buffer.f_frsize;
    unsigned long long used_space = total_space - free_space;
    return static_cast<int>(used_space / (1024 * 1024 * 1024));
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

int make_file(std::string filename) {
    if (!fs::exists(path + fs::path(filename).parent_path().string())) {
        std::cout << fs::path(filename).parent_path().string() << std::endl;
        std::cout << path + fs::path(filename).parent_path().string();
        fs::create_directories(path + fs::path(filename).parent_path().string());
    }
    write_file(path + filename, "");
    append_to_json("files.json", filename);
    return 0;
}

bool remove_entry_from_json_array(json& jsonArray, const std::string& entryToRemove) {

    for (auto it = jsonArray.begin(); it != jsonArray.end(); ++it) {
        if (*it == entryToRemove) {
            jsonArray.erase(it);
            return true;
        }
    }

    return false;
}

void delete_json(std::string filename) {
    std::string fileContent = read_file("files.json");
    json jsonArray = json::parse(fileContent);
    remove_entry_from_json_array(jsonArray, filename);
    write_file("files.json", jsonArray.dump(4));
}

int check_if_make_fille(std::string filename) {
    std::string fileContent = read_file("files.json");
    json jarray = json::parse(fileContent);
    for (std::string entry : jarray) {
        if (entry == filename) {
            return 0;
        }
    }
    make_file(trim_ex(filename));
    return 0;
}

std::string listFilesInFolder(const std::string& folderPath) {
    namespace fs = std::filesystem;
    nlohmann::json jsonArray = nlohmann::json::array();

    try {
        for (const auto& entry : fs::directory_iterator(path + folderPath)) {
            if (fs::is_regular_file(entry.path())) {
                jsonArray.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        return std::string("{\"error\": \"") + e.what() + "\"}";
    }

    return jsonArray.dump(4);
}

std::string list_files(std::string folder) {
    json files = json::parse("[]");
    std::string fileContent = read_file("files.json");
    json jarray = json::parse(fileContent);
    for (std::string entry : jarray) {
        if (is_in_string(entry, folder)) {
            std::cout << entry << std::endl;
            files.push_back(entry);
        }
    }
    return files.dump(4);
}

int main() {
    std::thread t(setup_for_file_cummonication);
    t.detach();
    auto socket_cum = std::make_shared<GigaSocketTLS>(
        wport, 10, "gigakey.key", "gigacert.crt", nullptr,
        [](SSL* ssl, int cumfd) {
            log("Closed client");
            m_connections.erase(ssl);
        },
        [](SSL* ssl, int cumfd) { log("opened socket"); });

    socket_cum->set_callback([socket_cum](SSL* ssl, int cumfd, std::string message) {
        log("got message: " + message);

        const std::string recvmsg = message;
        cout << recvmsg[0] << endl;
        auto it = filenames.find(ssl);
        if (it != filenames.end()) {
            std::ofstream file(path + it->second, std::ios::binary);
            file.write(recvmsg.c_str(), recvmsg.size());
            file.close();
            filenames.erase(ssl);
        }
        switch (recvmsg[0]) {
        case 'm':
            make_file(strip_first(recvmsg));
            break;
        case 'w': {
            write_file(path + read_until_char(strip_first(recvmsg), '%'), "");
            log("cleared file: " + path + read_until_char(strip_first(recvmsg), '%'));
            dateinamen_recv.insert({identnum, read_until_char(strip_first(recvmsg), '%')});
            chunks_recv.insert({identnum, stoi(read_from_char(strip_first(recvmsg), '%'))});
            socket_cum->send_msg(ssl, std::to_string(identnum), 't');
            if (identnum <= 999) {
                identnum = 99;
            }
            identnum++;
            break;
        }
        case 'l':{
            socket_cum->send_msg(ssl, listAllFilesAsJson(path + strip_first(recvmsg)), 't');
            log(listAllFilesAsJson(path + strip_first(recvmsg)));
            break;}
        case 'd': {
            std::remove((path + strip_first(recvmsg)).c_str());
            std::vector<std::string> folders = split_str(path + fs::path().parent_path().string(), '/');
            for (std::string folder : folders) {
                fs::remove(folder);
            }
            delete_json(trim_ex(strip_first(recvmsg)));
            break;
        }
        case 'a':
            socket_cum->send_msg(ssl, std::to_string(get_storage_free(path)), 't');
            break;
        case 'f':
            socket_cum->send_msg(ssl, std::to_string(get_storage_full(path)), 't');
            break;
        default:
            break;
        }
    });
for(;;){
    ;
    //waiting
}}