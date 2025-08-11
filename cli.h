// cli.h
// Created by zerox on 25-8-10.
//


#ifndef STRUCT_H
#define STRUCT_H
#include <string>
#include <filesystem>
#include <climits>

struct Options {
    std::filesystem::path path;
    std::string language;
    std::string option;
    unsigned int line = UINT_MAX;
    bool addl = false;
    bool updl = false;
    bool rmvl = false;
    bool delcl = false;





};

struct CliReturn {
    Options options;
    void (*funcPtr)(const Options&) = nullptr;
    bool success = false;  // 添加成功标志
};

CliReturn cli_work(int argc, char* argv[]);

#endif //STRUCT_H
