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

void send_file_chunker(int index, std::string filename)
{
    log("involved file chunker");
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    long chunks;
    if (file)
    {
        log("opened file sucessfully");
        log("file content");
        // log(file_data.c_str());
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
                // std::string substr = file_data.substr(i * chunksize, i* chunksize +chunksize);
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

void setup_for_file_cummonication()
{
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
    std::cout << recvmsg[0] << std::endl;
    auto entry = dateinamen_recv.find(recvmsg[0] - '0');
    for (const auto &pair : dateinamen_recv)
    {
        std::cout << pair.first << ":" << pair.second << std::endl;
    }
    log("entry filename" + entry->second);
    if (entry != dateinamen_recv.end())
    {
        log("entry filename" + entry->second);
        std::ofstream file(path + entry->second, std::ios::binary | std::ios::app);
        log("write to file: "+path + entry->second);
        file.write(strip_first(recvmsg).c_str(), recvmsg.size());
        file.close();
        auto entry_chunk = chunks_recv.find(recvmsg[0]);
        if (entry_chunk != chunks_recv.end())
        {
            entry_chunk->second--;
            if (entry_chunk->second <= 0)
            {
                log("try to delete entrys");
                dateinamen_recv.erase(recvmsg[0]-'0');
                chunks_recv.erase(recvmsg[0]-'0');
            }
        }
    }}

    } });

    auto res = file_server.listen();

    file_server.disablePerMessageDeflate();
    file_server.start();
    file_server.wait();
}
