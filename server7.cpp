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

using json = nlohmann::json;

#define wport 12357


std::string path = "/home/gigaserver/contents";
typedef websocketpp::server<websocketpp::config::asio_tls> server;

using websocketpp::connection_hdl;
using namespace std;
namespace fs = std::filesystem;
server m_server;

std::string trim_ex(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::map<connection_hdl, std::string, std::owner_less<connection_hdl>> filenames;
std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;

void log(std::string msg){
    std::cout<<msg<<std::endl;
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

bool remove_entry_from_json_array(json& jsonArray, const std::string& entryToRemove) {

    for (auto it = jsonArray.begin(); it != jsonArray.end(); ++it) {
        if (*it == entryToRemove) { 
            jsonArray.erase(it);    
            return true;           
        }
    }

    return false;
}

void send_file(websocketpp::connection_hdl hdl, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    
    if (file) {
        std::string file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        m_server.send(hdl, file_data, websocketpp::frame::opcode::binary);
        std::cout << "Datei \"" << filename << "\" erfolgreich gesendet." << std::endl;
    } 
}


void delete_json(std::string filename){
    std::string fileContent = read_file("files.json");
    json jsonArray = json::parse(fileContent);
    remove_entry_from_json_array(jsonArray, filename);
    write_file("files.json", jsonArray.dump(4));
}

int check_if_make_fille(std::string filename){
    std::string fileContent = read_file("files.json");
    json jarray = json::parse(fileContent);
    for(std::string entry : jarray){
            if(entry==filename){
                    return 0;
            }
    }
    make_file(trim_ex(filename));
    return 0;


}

std::string list_files(std::string folder){
    json files = json::parse("[]");
    std::string fileContent = read_file("files.json");
    json jarray = json::parse(fileContent);
    for(std::string entry : jarray){
        if(is_in_string(entry, folder)){
        std::cout<<entry<<std::endl;
        files.push_back(entry);}
    }
    std::string returner = files.dump(4);
    return returner;
}

void on_message(connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;
    const string recvmsg = msg->get_payload();
    //cout<<recvmsg<<endl;
    cout<<recvmsg[0]<<endl;
    auto it = filenames.find(hdl);
    if(it != filenames.end()){
        std::ofstream file(path+it->second,  std::ios::binary);
        file.write(recvmsg.c_str(), recvmsg.size());
        file.close();
        m_server.send(hdl, "d", websocketpp::frame::opcode::text);
        filenames.erase(hdl);
    }
    switch(recvmsg[0]){
        case 'm':
            make_file(strip_first(recvmsg));
            m_server.send(hdl, "d", websocketpp::frame::opcode::text);
            break;
        case 'w':{
            check_if_make_fille(trim_ex(strip_first(recvmsg)));
            filenames[hdl] = strip_first(recvmsg);
            break;}
        case 'r':
           send_file(hdl, path+strip_first(recvmsg));
            break;
        case 'l':
            m_server.send(hdl, list_files(strip_first(recvmsg)), websocketpp::frame::opcode::text);
            break;
        case 'd':{
            std::remove((path+strip_first(recvmsg)).c_str());
            std::vector<std::string> folders = split_str(path+fs::path().parent_path().string(), '/');
            for(std::string folder : folders){
                fs::remove(folder);
            }
            delete_json(trim_ex(strip_first(recvmsg)));
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