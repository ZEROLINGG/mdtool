// tools/tools.h
// Created by zerox on 25-8-10.
//

#ifndef TOOLS_H
#define TOOLS_H

#include "../cli.h"

namespace tools {
    enum path_info {
        FILE_MD,    // ����md�ĵ�
        FILE_OTHER, // ������md�ĵ�
        DIR_MD,    // ����md�ĵ���Ŀ¼ ��֧�ֶ༶Ŀ¼��
        DIR_OTHER,  // ������md�ĵ���Ŀ¼  ��֧�ֶ༶Ŀ¼��
        VERY_BIG,
        PATH_ERROR
    };

    void addl(const Options& Options);
    void updl(const Options& Options);
    void rmvl(const Options& Options);
    void delcl(const Options& Options);

}

#endif //TOOLS_H
