#include <cstdlib>
#include <string>
#include <iostream>

std::string download(const std::string& url, const std::string& downloadPath) {
    std::string outputTemplate = downloadPath + "/%(title)s.%(ext)s";
    std::string request = "yt-dlp -f mp4 -o \"" + outputTemplate + "\" \"" + url + "\"";
    int result = system(request.c_str());
    std::string fileName = downloadPath + "/%(title)s.mp4";  
    return fileName;
}

int main() {
    std::string url = "https://www.youtube.com/watch?v=1nd76F1quoo";
    std::string downloadPath = ".";
    std::string fileName = downloadAndGetFileName(url, downloadPath);
    
    if (!fileName.empty()) {
        std::cout << "Die Datei wurde heruntergeladen: " << fileName << std::endl;
    } else {
        std::cerr << "Fehler beim Download der Datei." << std::endl;
    }
    
    return 0;
}
