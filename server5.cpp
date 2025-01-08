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
#include <vector>

using json = nlohmann::json;

#define wport 12357
std::string path = "/home/gerbil/contents/";

typedef websocketpp::server<websocketpp::config::asio_tls> server;

using websocketpp::connection_hdl;
using namespace std;
namespace fs = std::filesystem;
server m_server;

std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;

int write_json(std::string filename, std::string msg){
    json data;
    json data1;
    data1 = json::parse(msg);
    data = json::parse(read_file(filename));
    data.push_back(data1);
    std::string jsonString = data.dump(4);
    write_file(filename, jsonString);
    return 0;
}

int make_file(std::string filename){
    if(startswith(filename, '/')){
        filename = strip_first(filename);
    }
    if(!fs::exists(path+fs::path(filename).parent_path().string())){
        std::cout<<fs::path(filename).parent_path().string()<<std::endl;
        std::cout<<path+fs::path(filename).parent_path().string();
        fs::create_directories(path+fs::path(filename).parent_path().string());
    }
    write_file(path+filename, "");
    write_json("files.txt", filename);
    return 0;
}

std::shared_ptr<boost::asio::ssl::context> on_tls_init(websocketpp::connection_hdl hdl) {
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


void on_open(connection_hdl hdl) {
    m_connections.insert(hdl);
    std::cout << "Client connected! Total clients: " << m_connections.size() << std::endl;
}

void on_close(connection_hdl hdl) {
    m_connections.erase(hdl);
    std::cout << "Client disconnected! Total clients: " << m_connections.size() << std::endl;
}

void on_message(connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;
    const string recvmsg = msg->get_payload();
    cout<<recvmsg<<endl;
    cout<<recvmsg[0]<<endl;
    switch(recvmsg[0]){
        case 'm':
            make_file(strip_first(recvmsg));
            m_server.send(hdl, "d", websocketpp::frame::opcode::text);
            break;
        case 'w':
            write_file(read_until_char(strip_first(recvmsg), '%'), read_from_char(strip_first(recvmsg), '%'));
            m_server.send(hdl, "d", websocketpp::frame::opcode::text);
            break;
        case 'r':
            m_server.send(hdl, read_file(path+strip_first(recvmsg)), websocketpp::frame::opcode::text);
            break;
        case 'l':
            m_server.send(hdl, read_file("files.txt"), websocketpp::frame::opcode::text);
            break;
        case 'd':{
            std::remove((path+strip_first(recvmsg)).c_str());
            std::vector<std::string> folders = split_str(path+fs::path().parent_path().string(), '/');
            for(std::string folder : folders){
                fs::remove(folder);
            }
            m_server.send(hdl, "d", websocketpp::frame::opcode::text);
            break;}
        default:
            break;
    }
}
    
int main() {
    m_server.init_asio();
    m_server.set_tls_init_handler(bind(&on_tls_init, std::placeholders::_1));
    m_server.set_open_handler(bind(&on_open, std::placeholders::_1));
    m_server.set_close_handler(bind(&on_close, std::placeholders::_1));
    m_server.set_message_handler(bind(&on_message, std::placeholders::_1, std::placeholders::_2));
    m_server.listen(wport);
    m_server.start_accept();
    m_server.run();
}