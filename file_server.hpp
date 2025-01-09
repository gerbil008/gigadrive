//g++ -std=c++11 server-websocket.cpp -o server -lboost_system -lpthread
//y=true, n=false, c=how many clients, j=check json, s=send json, r=receive new json
#include <websocketpp/config/asio.hpp> 
#include <websocketpp/server.hpp>
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

#define wport 12369
#define wport_files 12368

std::mutex _var_lock;

typedef websocketpp::server<websocketpp::config::asio_tls> server;

using websocketpp::connection_hdl;
using namespace std;
namespace fs = std::filesystem;
server m_server_files;

const std::string path = "/home/gerbil/contents";

std::map<int, std::string> dateinamen_recv;
std::map<int, int> chunks_recv;
std::map<int, std::string> dateinamen_send;


std::string trim_ex(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections_files;

void log(std::string msg){
    std::cout<<msg<<std::endl;
}


std::shared_ptr<boost::asio::ssl::context> on_tls_init_file(websocketpp::connection_hdl hdl) {
    auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->use_certificate_chain_file("fullchain.pem");  
        ctx->use_private_key_file("privkey.pem", boost::asio::ssl::context::pem);  

    } catch (std::exception& e) {
        std::cout << "Error setting up TLS: " << e.what() << std::endl;
    }

    return ctx;
}

void on_open_file(connection_hdl hdl) {
    m_connections_files.insert(hdl);
    std::cout << "Client file connected! Total clients: " << m_connections_files.size() << std::endl;
}

void on_close_file(connection_hdl hdl) {
    m_connections_files.erase(hdl);
    std::cout << "Client file disconnected! Total clients: " << m_connections_files.size() << std::endl;
}


void send_file(websocketpp::connection_hdl hdl, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    
    if (file) {
        std::string file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        m_server_files.send(hdl, file_data, websocketpp::frame::opcode::binary);
        std::cout << "Datei \"" << filename << "\" erfolgreich gesendet." << std::endl;
    } 
}


void send_file_in_chunks(websocketpp::connection_hdl hdl, const std::string& file_path) {
    const size_t chunk_size = 2 * 1024 * 1024; // 2 MB chunks
    std::ifstream file(file_path, std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    size_t total_chunks = (file_size + chunk_size - 1) / chunk_size;
    file.seekg(0, std::ios::beg);


    size_t current_chunk = 0;
    std::vector<char> buffer(chunk_size);

    while (file) {
        file.read(buffer.data(), chunk_size);
        std::streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            m_server_files.send(hdl, buffer.data(), bytes_read, websocketpp::frame::opcode::binary);
            current_chunk++;
            std::cout << "Sent chunk " << current_chunk << " of " << total_chunks << std::endl;
        }
    }

    // Notify the client that the transfer is complete
    m_server_files.send(hdl, "e", websocketpp::frame::opcode::text);
    std::cout << "File transfer complete." << std::endl;
}


void on_message_file(connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Received message by file server" << msg->get_payload()<< std::endl;
    const string recvmsg = msg->get_payload();
    std::cout<<recvmsg[0]<<std::endl;
    auto entry = dateinamen_recv.find(recvmsg[0] - '0');
    for (const auto& pair : dateinamen_recv) {
        std::cout << pair.first << ":" << pair.second << std::endl;
    }
    log("entry filename"+entry->second);
    if(entry != dateinamen_recv.end()){
        log("entry filename"+entry->second);
        std::ofstream file(path+entry->second,  std::ios::binary | std::ios::app);
        file.write(strip_first(recvmsg).c_str(), recvmsg.size());
        file.close();
        auto entry_chunk = chunks_recv.find(recvmsg[0]);
        if(entry_chunk != chunks_recv.end()){entry_chunk->second--;
        if(entry_chunk->second <= 0){
            dateinamen_recv.erase(recvmsg[0]);
            chunks_recv.erase(recvmsg[0]);
        }}
}
}

void setup_for_file_cummonication(){
    log("Start setup file server");
    m_server_files.clear_access_channels(websocketpp::log::alevel::all);
    m_server_files.clear_error_channels(websocketpp::log::elevel::all);
    m_server_files.init_asio();
    m_server_files.set_tls_init_handler(bind(&on_tls_init_file, std::placeholders::_1));
    m_server_files.set_open_handler(bind(&on_open_file, std::placeholders::_1));
    m_server_files.set_close_handler(bind(&on_close_file, std::placeholders::_1));
    m_server_files.set_message_handler(bind(&on_message_file, std::placeholders::_1, std::placeholders::_2));
    m_server_files.listen(wport_files);
    m_server_files.start_accept();
    m_server_files.run();
}

