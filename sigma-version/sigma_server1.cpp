#include "sigma_fileserver2.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

beast::ssl_stream<beast::tcp_stream> cumstream;
websocket::stream<beast::ssl_stream<beast::tcp_stream>> cumws;

beast::flat_buffer cumbuffer;

std::map<websocket::stream<beast::ssl_stream<beast::tcp_stream>>, std::string> filenames;
std::set<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> m_connections;

int identnum = 100;

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

int routine() {
    std::string cumstr = boost::beast::buffers_to_string(filebuffer.data());
    std::cout << "Received: " << cumstr << std::endl;

    const std::string recvmsg = cumstr;
    cout << recvmsg[0] << endl;
    auto it = filenames.find(cumws);
    if (it != filenames.end()) {
        std::ofstream file(path + it->second, std::ios::binary);
        file.write(recvmsg.c_str(), recvmsg.size());
        file.close();
        filenames.erase(cumws);
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
        cumws.write(std::to_string(identnum));
        if (identnum <= 999) {
            identnum = 99;
        }
        identnum++;
        break;
    }
    case 'l':
        cumws.write(listAllFilesAsJson(path + strip_first(recvmsg)));
        log(listAllFilesAsJson(path + strip_first(recvmsg)));
        break;
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
        cumws.write(std::to_string(get_storage_free(path)));
        break;
    case 'f':
        cumws.write(std::to_string(get_storage_full(path)));
        break;
    default:
        break;
    }
}

void handle_session(tcp::socket socket, net::ssl::context& ctx) {
    try {
        beast::ssl_stream<beast::tcp_stream> stream(std::move(socket), ctx);

        stream.handshake(net::ssl::stream_base::server);
        websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws(std::move(stream));
        ws.accept();

        beast::flat_buffer buffer;
        while (true) {
            ws.read(buffer);
            routine();
            buffer.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

int setup_for_cummonication() {
    try {
        net::io_context ioc;
        net::ssl::context ctx(net::ssl::context::tlsv12);

        ctx.use_certificate_file("server.crt", net::ssl::context::pem);
        ctx.use_private_key_file("server.key", net::ssl::context::pem);

        tcp::acceptor acceptor(ioc, {tcp::v4(), wport});
        std::cout << "WebSocket-Server läuft auf wss://localhost:8080" << std::endl;

        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread(handle_session, std::move(socket), std::ref(ctx)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}

int main() {
    std::thread t(setup_fileserver);
    t.detach();
    setup_for_cummonication();
}
