//
// Created by zerox on 25-8-10.
//

#ifndef PATH_H
#define PATH_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include <iostream>
std::filesystem::path to_fs_path(const std::string& path_str);
std::string out_fs_path(const std::filesystem::path& path);


#endif //PATH_H
