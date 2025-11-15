//
// Created by zerox on 2025/11/10.
//

#ifndef MDTOOL2_CODEBLOCK_H
#define MDTOOL2_CODEBLOCK_H

#include <re2/re2.h>
#include "../../global.h"


// md代码块操作类
class CodeBlock {
    static const RE2 codeBlockRegex;
    static encoding enc;
    class LanguageIdentifier {
    public:
        FinalFuncReturn add(const fs::path& path, const std::string& language, const int& start);
        bool upd(const fs::path& path, const std::string& language);
        bool del(const fs::path& path);
        bool format(const fs::path& path);  // 去除多余空白字符

    };

    class CodeContent {
    public:
        bool add(const fs::path& path, const std::string& content);
        bool del(const fs::path& path);
        bool format(const fs::path& path);
    };

public:
     CodeBlock() {}
    LanguageIdentifier li;
    CodeContent ct;
};


#endif //MDTOOL2_CODEBLOCK_H