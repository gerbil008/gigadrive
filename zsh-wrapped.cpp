#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#define path "/home/gerbil/.zhistory"

std::map<std::string, int> commands;

std::vector<std::string> split_str(std::string str, char del){
    std::string j = "";
    std::vector<std::string> re;
    for(char i : str){
        if(i != del){
            j += i;
        }
        else{
            re.push_back(j);
            j = "";
        }
    }
    re.push_back(j);
    return re;
}


int main(void){
    std::string line;
    std::ifstream file(path);

    while (std::getline(file, line)){
        std::vector<std::string> line1 = split_str(line, ' ');
        if(line1[0] != "sudo"){
            if(commands.find(line1[0]) != commands.end()){
                commands[line1[0]] += 1;
            }
            else{
                commands[line1[0]] = 1;
            }
        }
        else if(line1[0] == "sudo"){
            if(commands.find(line1[1]) != commands.end()){
                commands[line1[1]] += 1;
            }
            else{
                commands[line1[1]] = 1;
            }
        }
    }
    file.close();

    std::vector<std::pair<std::string, int>> sorted(commands.begin(), commands.end());

    std::sort(sorted.begin(), sorted.end(),[](const auto& a, const auto& b) { return a.second < b.second; });

    for (const auto& pair : sorted) {
        if(pair.first != ""){
            std::cout << pair.first << ": " << pair.second << "\n";
        }
    }
    
    return 0;
}