// tools/tools.h
// Created by zerox on 25-8-10.
//

#ifndef TOOLS_H
#define TOOLS_H

#include "../cli.h"

namespace tools {
    enum path_info {
        FILE_MD,    // 单个md文档
        FILE_OTHER, // 单个非md文档
        DIR_MD,    // 含有md文档的目录 （支持多级目录）
        DIR_OTHER,  // 不含有md文档的目录  （支持多级目录）
        VERY_BIG,
        PATH_ERROR
    };

    void addl(const Options& Options);
    void updl(const Options& Options);
    void rmvl(const Options& Options);
    void delcl(const Options& Options);

}

#endif //TOOLS_H
