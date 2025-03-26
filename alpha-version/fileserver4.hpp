#include "gigasocket.hpp"
#include <algorithm>
#include <boost/asio/ssl/context.hpp>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ixwebsocket/IXWebSocketServer.h>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string_ops.h>
#include <sys/statvfs.h>
#include <thread>
#include <vector>

using json = nlohmann::json;

const int chunksize = 2 * 1024 * 1024;

#define wport 12369
#define wport_files 12368
#define send_delay 200

bool line_free = true;
int ident = 1;

std::mutex _var_lock;

using namespace std;
namespace fs = std::filesystem;

const std::string path = "/home/gerbil/contents";

std::map<int, std::string> dateinamen_recv;
std::map<int, int> chunks_recv;
std::map<int, std::string> dateinamen_send;
std::vector<SSL*> active_conns;
std::string ca_path = "";
GigaSocketTLS* file_socket_ptr = nullptr;

void log(std::string msg) {
    std::cout << msg << std::endl;
}

std::string trim_ex(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

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

std::string listAllFilesAsJson(const std::string& folderPath1) {
    const std::string folderPath = trim_ex(folderPath1);
    if (!fs::exists(folderPath)) {
        std::cerr << "Fehler: Verzeichnis existiert nicht -> " << folderPath << std::endl;
        return "{}";
    }

    nlohmann::json jsonArray = nlohmann::json::array();
    fs::path basePath = fs::absolute(folderPath);  // Sicherstellen, dass der Pfad absolut ist

    log("Starte Suche nach 'contents' ab: " + basePath.string());

    while (basePath.has_parent_path() && basePath.filename() != "contents") {
        fs::path parent = basePath.parent_path();

        if (parent == basePath) {  // Falls Root-Verzeichnis erreicht wurde
            std::cerr << "Fehler: Root-Verzeichnis erreicht, 'contents' nicht gefunden!\n";
            return "{}";
        }

        basePath = parent;
        log("Neuer basePath: " + basePath.string());
    }

    if (basePath.filename() != "contents") {
        std::cerr << "Fehler: 'contents' Verzeichnis nicht gefunden!\n";
        return "{}";
    }

    log("Gefundenes 'contents'-Verzeichnis: " + basePath.string());

    listFilesRecursive(folderPath, basePath, jsonArray);
    return jsonArray.dump(4);
}


int web_send(int index, std::string str) {
    try {
        // active_conns[index]->send(str, true);
        file_socket_ptr->send_msg(active_conns[index], str, 'b');
        return 0;
    } catch (...) {
        return 1;
    }
}

int get_ident(std::string& msg) {

    std::string first = msg.substr(0, 3);
    int number;
    std::istringstream(first) >> number;
    return number;
}

void send_file_chunker(int index, std::string filename) {
    log("involved file chunker");
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    long chunks;
    if (file) {
        log("opened file sucessfully");
        log("file content");
        long double size = file.tellg();
        long long test_int;
        if (size > chunksize) {
            log(std::to_string(size / chunksize));
            log(std::to_string(size));
            test_int = size / chunksize;
            log(std::to_string(test_int));
            chunks = size / chunksize;
            if (test_int < size / chunksize) {
                chunks++;
            }
            for (int i = 0; i < chunks; i++) {
                log("Chunk " + std::to_string(i + 1) + " out of " + std::to_string(chunks));
                char buf[chunksize];
                file.seekg(i * chunksize);
                std::string substr;
                if (chunks - 1 == i) {
                    file.read(buf, size - i * chunksize);
                    substr = std::string(buf, size - i * chunksize);
                } else {
                    file.read(buf, chunksize);
                    substr = std::string(buf, chunksize);
                }
                if (web_send(index, substr) == 1) {
                    log("Sending error because of closed connection");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(send_delay));
            }
            try {
                // active_conns[index]->send("e");
                file_socket_ptr->send_msg(active_conns[index], "e", 't');
            } catch (...) {
                log("exit send shit in");
            }
        }

        else if (size <= chunksize) {
            file.seekg(0);
            std::string file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            web_send(index, file_data);
            log(file_data);
            log("send file data");
            std::this_thread::sleep_for(std::chrono::milliseconds(send_delay));
            try {
                // active_conns[index]->send("e");
                file_socket_ptr->send_msg(active_conns[index], "e", 't');
            } catch (...) {
                ;
            }
        }
        file.close();
    }
}

std::string download(const std::string& url, const std::string& downloadPath, const int ident) {
    std::string outputTemplate = downloadPath + "/" + std::to_string(ident) + "%(title)s.%(ext)s";
    std::string request = "yt-dlp -f mp4 -o \"" + outputTemplate + "\" \"" + url + "\"";
    int result = system(request.c_str());
    std::string fileName = downloadPath + "/%(title)s.mp4";
    return fileName;
}

std::string download_mp3(const std::string& url, const std::string& downloadPath, const int ident) {
    std::string outputTemplate = downloadPath + "/" + std::to_string(ident) + "%(title)s.%(ext)s";
    std::string request =
        "yt-dlp  -f bestaudio --extract-audio --audio-format mp3 -o \"" + outputTemplate + "\" \"" + url + "\"";

    // std::string request = "yt-dlp -f mp4 -o \"" + outputTemplate + "\" \"" + url + "\"";
    int result = system(request.c_str());
    std::string fileName = downloadPath + "/%(title)s.mp4";
    return fileName;
}

std::string strip_ident(std::string msg) {
    if (msg.size() > 3) {
        msg.erase(0, 3);
    }
    return msg;
}

std::vector<std::string> list_files_in_directory(const std::string& directory_path) {
    std::vector<std::string> file_list;
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            file_list.push_back(entry.path().string());
        }
    }
    return file_list;
}

void setup_for_file_cummonication() {
    auto file_socket = std::make_shared<GigaSocketTLS>(
        wport_files, 10, "gigakey.key", "gigacert.crt", nullptr,
        [](SSL* ssl, int cumfd) {
            log("Closed client");
            // m_connections.erase(ssl);
        },
        [](SSL* ssl, int cumfd) {
            log("opened socket");
            std::cout << "New connection" << std::endl;
            {
                std::lock_guard<std::mutex> lock(_var_lock);
                active_conns.push_back(ssl);
            }
        });

    file_socket->set_callback([file_socket](SSL* ssl, int cumfd, std::string message) {
        log("got message: " + message);
        if (message != "p69") {
            std::cout << "Received: " << message << std::endl;
            std::string recvmsg = message;
            recvmsg = trim_ex(recvmsg);
            if (recvmsg[0] == 'r') {
                auto it = std::find(active_conns.begin(), active_conns.end(), ssl);

                if (it != active_conns.end()) {

                    int index = std::distance(active_conns.begin(), it);
                    std::thread cunker(send_file_chunker, index, path + strip_first(recvmsg));
                    cunker.detach();
                }
                // send_file_chunker(webSocket, path + strip_first(recvmsg));
                log("try_send_file");
            }

            if (recvmsg[0] == 'w') {
                std::string link = strip_first(recvmsg);
                std::string loc = download_mp3(link, path + "/Youtube-Downloads", ident);
                const int ident1 = ident;
                ident++;
                if (ident >= 999) {
                    ident = 1;
                }

                std::string loc1 = "";

                std::vector<std::string> names = list_files_in_directory(path + "/Youtube-Downloads");
                for (std::string filename : names) {
                    std::vector<std::string> filename1 = split_str(filename, '/');
                    if (filename1[filename1.size() - 1][0] - '0' == ident1) {
                        log(filename);
                        loc = filename;
                        loc1 = filename1[filename1.size() - 1];
                    }
                }

                file_socket->send_msg(ssl, loc1, 't');
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto it = std::find(active_conns.begin(), active_conns.end(), ssl);

                if (it != active_conns.end()) {

                    int index = std::distance(active_conns.begin(), it);
                    std::thread cunker(send_file_chunker, index, loc);
                    cunker.detach();
                }
                // send_file_chunker(webSocket, path + strip_first(recvmsg));
                log("try_send_file");
            }

            if (recvmsg[0] == 's') {
                std::string link = strip_first(recvmsg);
                std::string loc = download(link, path + "/Youtube-Downloads", ident);
                const int ident1 = ident;
                ident++;
                if (ident >= 999) {
                    ident = 1;
                }

                std::string loc1 = "";

                std::vector<std::string> names = list_files_in_directory(path + "/Youtube-Downloads");
                for (std::string filename : names) {
                    std::vector<std::string> filename1 = split_str(filename, '/');
                    if (filename1[filename1.size() - 1][0] - '0' == ident1) {
                        log(filename);
                        loc = filename;
                        loc1 = filename1[filename1.size() - 1];
                    }
                }

                file_socket->send_msg(ssl, loc1, 't');
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto it = std::find(active_conns.begin(), active_conns.end(), ssl);

                if (it != active_conns.end()) {

                    int index = std::distance(active_conns.begin(), it);
                    std::thread cunker(send_file_chunker, index, loc);
                    cunker.detach();
                }
                // send_file_chunker(webSocket, path + strip_first(recvmsg));
                log("try_send_file");
            }

            std::cout << recvmsg[0] << std::endl;
            auto entry = dateinamen_recv.find(get_ident(recvmsg));
            for (const auto& pair : dateinamen_recv) {
                std::cout << pair.first << ":" << pair.second << std::endl;
            }
            log("entry filename" + entry->second);
            if (entry != dateinamen_recv.end()) {
                log("entry filename" + entry->second);
                std::ofstream file(path + entry->second, std::ios::binary | std::ios::app);
                log("write to file: " + path + entry->second);
                log("stripped ident: " + strip_ident(recvmsg));
                file.write(strip_ident(recvmsg).c_str(), recvmsg.size() - 3);
                file.close();
                auto entry_chunk = chunks_recv.find(get_ident(recvmsg));
                if (entry_chunk != chunks_recv.end()) {
                    log("we are in entry chunks");
                    entry_chunk->second--;
                    if (entry_chunk->second <= 0) {
                        log("try to delete entrys");
                        dateinamen_recv.erase(get_ident(recvmsg));
                        chunks_recv.erase(get_ident(recvmsg));
                        file_socket->send_msg(ssl, "d", 't');
                    }
                }
            }
        }
    });
}
