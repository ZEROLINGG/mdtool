//
// Created by zerox on 2025/11/7.
//

#ifndef MDTOOL2_CLI_H
#define MDTOOL2_CLI_H

#include <cxxopts.hpp>

#include "global.h"


// 命令行解析后返回的统一结构体
struct CommandReturn {
    InputOptions options;
    FinalFunc funcPtr = nullptr;
    bool success = false;  // 添加成功标志
};

struct oldOptions {
    bool addl;
    bool updl;
    bool rmvl;
    bool delcl;
};









#endif //MDTOOL2_CLI_H