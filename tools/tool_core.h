//
// Created by zerox on 25-8-10.
//

#ifndef TOOL_CORE_H
#define TOOL_CORE_H
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

bool addl_(const fs::path& path,const std::string& language);
bool updl_(const fs::path& path, const std::string& language);
bool rmvl_(const fs::path& path);
bool delcl_(const fs::path& path, const unsigned int& line);

#endif //TOOL_CORE_H
