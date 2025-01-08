#include <iostream>
#include <sys/statvfs.h>

int get_storage_free(const std::string& path) {
    struct statvfs buffer;
    statvfs(path.c_str(), &buffer);
    unsigned long long total_space = buffer.f_blocks * buffer.f_frsize;
    return static_cast<int>(total_space / (1024 * 1024 * 1024)); // Umrechnung in GB
}

int get_storage_full(const std::string& path) {
    struct statvfs buffer;
    statvfs(path.c_str(), &buffer);
    unsigned long long total_space = buffer.f_blocks * buffer.f_frsize;
    unsigned long long free_space = buffer.f_bfree * buffer.f_frsize;
    unsigned long long used_space = total_space - free_space;
    return static_cast<int>(used_space / (1024 * 1024 * 1024)); // Umrechnung in GB
}

int main() {
    std::cout << "Free space (GB): " << get_storage_free("/") << std::endl;
    std::cout << "Used space (GB): " << get_storage_full("/") << std::endl;
    return 0;
}
