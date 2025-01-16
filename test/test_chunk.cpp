#include <iostream>
#include <set>
#include <thread>
#include <stdio.h>
#include <string>
#include <fstream>
#include <string_ops.h>
#include <cstring>

#define chunksize 200

void log(std::string msg)
{
    std::cout << msg << std::endl;
}

int send(std::string msg)
{
    m_server_files.send(hdl, msg,  websocketpp::frame::opcode::binary);
    return 0;
}

void send_file(const std::string filename)
{
    std::ifstream file(filename, std::ios::binary);
    long chunks;
    if (file)
    {
        std::string file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        long long test_int;
        long double size = file_data.size();
        if (size > chunksize)
        {
            log(std::to_string(size/chunksize));
            log(std::to_string(size));
            test_int = size/chunksize;
            log(std::to_string(test_int));
            chunks = size/chunksize;
            if(test_int < size/chunksize){
                chunks++;
            }
            for(int i = 0; i < chunks; i++){
                log("Chunk "+std::to_string(i+1)+" out of "+std::to_string(chunks));
                std::string substr = file_data.substr(i*chunksize, +chunksize);
                send(substr);
            }
            send("e");
        }

        else if (size <= chunksize)
        {
            send(file_data);
            send("e");
        }
    }
}

int main(void)
{
    send_file("a.out");
    return 0;
}