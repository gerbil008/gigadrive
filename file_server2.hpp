#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>
#include <set>
#include <thread>
#include <stdio.h>
#include <string>
#include <fstream>
#include <boost/asio/ssl/context.hpp>
#include <string_ops.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <vector>
#include <map>
#include <memory>
#include <sys/statvfs.h>
#include <mutex>
#include <sstream>

using json = nlohmann::json;

const int chunksize = 2 * 1024 * 1024;

#define wport 12369
#define wport_files 12368
#define send_delay 200

bool line_free = true;

std::mutex _var_lock;

using namespace std;
namespace fs = std::filesystem;

const std::string path = "/home/gerbil/contents";

std::map<int, std::string> dateinamen_recv;
std::map<int, int> chunks_recv;
std::map<int, std::string> dateinamen_send;
std::vector<ix::WebSocket *> active_conns;
std::string ca_path = "";

void log(std::string msg)
{
    std::cout << msg << std::endl;
}

std::string trim_ex(const std::string &str)
{
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

std::string host("0.0.0.0");
ix::WebSocketServer file_server(wport_files, host, ix::SocketServer::kDefaultTcpBacklog, ix::SocketServer::kDefaultMaxConnections, 1800, ix::SocketServer::kDefaultAddressFamily);

int web_send(int index, std::string str)
{
    try
    {
        active_conns[index]->send(str, true);
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

int get_ident(std::string &msg){

    std::string first = msg.substr(0, 3);
    int number;
    std::istringstream(first) >> number;
    return number;
}

void send_file_chunker(int index, std::string filename)
{
    log("involved file chunker");
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    long chunks;
    if (file)
    {
        log("opened file sucessfully");
        log("file content");
        long double size = file.tellg();
        long long test_int;
        if (size > chunksize)
        {
            log(std::to_string(size / chunksize));
            log(std::to_string(size));
            test_int = size / chunksize;
            log(std::to_string(test_int));
            chunks = size / chunksize;
            if (test_int < size / chunksize)
            {
                chunks++;
            }
            for (int i = 0; i < chunks; i++)
            {
                log("Chunk " + std::to_string(i + 1) + " out of " + std::to_string(chunks));
                char buf[chunksize];
                file.seekg(i * chunksize);
                std::string substr;
                if (chunks - 1 == i)
                {
                    file.read(buf, size - i * chunksize);
                    substr = std::string(buf, size - i * chunksize);
                }
                else
                {
                    file.read(buf, chunksize);
                    substr = std::string(buf, chunksize);
                }
                if (web_send(index, substr) == 1)
                {
                    log("Sending error because of closed connection");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(send_delay));
            }
            try
            {
                active_conns[index]->send("e");
            }
            catch (...)
            {
                log("exit send shit in");
            }
        }

        else if (size <= chunksize)
        {
            file.seekg(0);
            std::string file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            web_send(index, file_data);
            log(file_data);
            log("send file data");
            std::this_thread::sleep_for(std::chrono::milliseconds(send_delay));
            try
            {
                active_conns[index]->send("e");
            }
            catch (...)
            {
                ;
            }
        }
        file.close();
    }
}

std::string download(const std::string& url, const std::string& downloadPath) {
    std::string outputTemplate = downloadPath + "/%(title)s.%(ext)s";
    std::string request = "yt-dlp -f mp4 -o \"" + outputTemplate + "\" \"" + url + "\"";
    int result = system(request.c_str());
    std::string fileName = downloadPath + "/%(title)s.mp4";  
    return fileName;
}

std::string strip_ident(std::string msg){
    if (msg.size() > 3) {
        msg.erase(0, 3); 
    }
    return msg;
}

void setup_for_file_cummonication()
{
    ix::SocketTLSOptions tlsOptions;
    tlsOptions.certFile = "fullchain.pem";
    tlsOptions.keyFile = "privkey.pem";
    tlsOptions.caFile = "fullchain.pem";

    file_server.setTLSOptions(tlsOptions);

    file_server.setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket &webSocket, const ix::WebSocketMessagePtr &msg)
                                           {
    webSocket.setPingInterval(3);
    if (msg->type == ix::WebSocketMessageType::Open)
    {
        std::cout << "New connection" << std::endl;

        std::cout << "id: " << connectionState->getId() << std::endl;
        {
    std::lock_guard<std::mutex> lock(_var_lock);
    active_conns.push_back(&webSocket);
}


    }

    else if(msg->type == ix::WebSocketMessageType::Close || msg->type == ix::WebSocketMessageType::Error){
        log("connection closed with id: ");
        std::cout << connectionState->getId() << std::endl;
    }

    else if (msg->type == ix::WebSocketMessageType::Message)
    {
        if(msg->str != "p69"){
        std::cout << "Received: " << msg->str << std::endl;
    std::string recvmsg = msg->str;
    recvmsg = trim_ex(recvmsg);
    if (recvmsg[0] == 'r')
    {
        auto it = std::find(active_conns.begin(), active_conns.end(), &webSocket);

        if (it != active_conns.end()) {

        int index = std::distance(active_conns.begin(), it);
        std::thread cunker(send_file_chunker, index, path + strip_first(recvmsg));
        cunker.detach();}
        //send_file_chunker(webSocket, path + strip_first(recvmsg));
        log("try_send_file");

    }

    if(recvmsg[0] == 's'){
        std::string link = strip_first(recvmsg);
        std::string loc = download(link, path+"/Youtube-Downloads");
        auto it = std::find(active_conns.begin(), active_conns.end(), &webSocket);

        if (it != active_conns.end()) {

            int index = std::distance(active_conns.begin(), it);
            std::thread cunker(send_file_chunker, index, loc);
            cunker.detach();}
        //send_file_chunker(webSocket, path + strip_first(recvmsg));
        log("try_send_file");
    }

    std::cout << recvmsg[0] << std::endl;
    auto entry = dateinamen_recv.find(get_ident(recvmsg));
    for (const auto &pair : dateinamen_recv)
    {
        std::cout << pair.first << ":" << pair.second << std::endl;
    }
    log("entry filename" + entry->second);
    if (entry != dateinamen_recv.end())
    {
        log("entry filename" + entry->second);
        std::ofstream file(path + entry->second, std::ios::binary | std::ios::app);
        log("write to file: " + path + entry->second);
        log("stripped ident: "+strip_ident(recvmsg));
        file.write(strip_ident(recvmsg).c_str(), recvmsg.size()-3);
        file.close();
        auto entry_chunk = chunks_recv.find(get_ident(recvmsg));
        if (entry_chunk != chunks_recv.end())
        {
            log("we are in entry chunks");
            entry_chunk->second--;
            if (entry_chunk->second <= 0)
            {
                log("try to delete entrys");
                dateinamen_recv.erase(get_ident(recvmsg));
                chunks_recv.erase(get_ident(recvmsg));
                webSocket.send("d");
            }
        }
    }}

    } });
    auto res = file_server.listen();

    file_server.disablePerMessageDeflate();
    file_server.start();
    file_server.wait();
}
