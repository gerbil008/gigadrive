#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
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

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

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

std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> filestream;

std::shared_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> filews;

beast::flat_buffer filebuffer;

std::map<int, std::string> dateinamen_recv;
std::map<int, int> chunks_recv;
std::map<int, std::string> dateinamen_send;
std::string ca_path = "";
std::vector<std::shared_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>> active_conns;

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

int get_ident(std::string& msg) {

    std::string first = msg.substr(0, 3);
    int number;
    std::istringstream(first) >> number;
    return number;
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

int web_send(int index, std::string str) {
    try {
        active_conns[index]->write(net::buffer(str));
        return 0;
    } catch (...) {
        return 1;
    }
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
                active_conns[index]->write(net::buffer("e"));
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
                active_conns[index]->write(net::buffer("e"));
            } catch (...) {
                ;
            }
        }
        file.close();
    }
}

int file_routine() {
    std::string filestr = boost::beast::buffers_to_string(filebuffer.data());
    if (filestr != "p69") {
        std::cout << "Received: " << filestr << std::endl;
        std::string recvmsg = filestr;
        recvmsg = trim_ex(recvmsg);
        if (recvmsg[0] == 'r') {
            auto it = std::find(active_conns.begin(), active_conns.end(), filews);

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

            filews->write(net::buffer(loc1));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto it = std::find(active_conns.begin(), active_conns.end(), filews);

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

            filews->write(net::buffer(loc1));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto it = std::find(active_conns.begin(), active_conns.end(), filews);

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
                    filews->write(net::buffer("d"));
                }
            }
        }
    }
    return 0; // Add a return statement to ensure the function returns an integer value
}

void handle_filesession(tcp::socket socket, net::ssl::context& ctx) {
    try {
        auto filestream = std::make_shared<beast::ssl_stream<beast::tcp_stream>>(std::move(socket), ctx);

        filestream->handshake(net::ssl::stream_base::server);
        auto filews = std::make_shared<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(std::move(*filestream));
        filews->accept();

        while (true) {
            filews->read(filebuffer);
            file_routine();
            filebuffer.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

int setup_fileserver() {
    try {
        net::io_context ioc;
        net::ssl::context ctx(net::ssl::context::tlsv12);

        ctx.use_certificate_file("server.crt", net::ssl::context::pem);
        ctx.use_private_key_file("server.key", net::ssl::context::pem);

        tcp::acceptor acceptor(ioc, {tcp::v4(), wport_files});

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread(handle_filesession, std::move(socket), std::ref(ctx)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}