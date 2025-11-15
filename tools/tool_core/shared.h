//
// Created by zerox on 2025/11/10.
//
//
#ifndef MDTOOL2_SHARED_H
#define MDTOOL2_SHARED_H

#include <uchardet/uchardet.h>
#include <iconv.h>
#include <memory>
#include <string>
#include <optional>
#include <tuple> // 引入 tuple
#include "../../global.h"

// 标准化换行符
std::string normalize_newlines(const std::string& str);

#define BUFFER_SIZE 65536
#define MAX_DETECTION_SIZE (BUFFER_SIZE * 49)

// RAII 封装用于自动资源管理
struct UchardetDeleter {
    void operator()(uchardet_t ud) const {
        if (ud) uchardet_delete(ud);
    }
};

struct IconvDeleter {
    void operator()(iconv_t cd) const {
        if (cd != reinterpret_cast<iconv_t>(-1)) {
            iconv_close(cd);
        }
    }
};

struct CStringDeleter {
    void operator()(char* ptr) const {
        if (ptr) free(ptr);
    }
};

using UchardetPtr = std::unique_ptr<std::remove_pointer<uchardet_t>::type, UchardetDeleter>;
using IconvPtr = std::unique_ptr<std::remove_pointer<iconv_t>::type, IconvDeleter>;
using CStringPtr = std::unique_ptr<char, CStringDeleter>;

class encoding {
public:
    // 返回 optional 而不是裸指针，避免内存管理问题
    std::optional<std::string> detect_file_charset(const char *filename);

    // 使用 optional 表示可能失败的操作
    std::optional<std::string> readToUtf8(const char *filename, std::string charset);
    std::optional<std::string> readToUtf8(const char *filename);

    bool saveUtf8ToFile(const char *filename, const std::string& data,
                        const std::string& charset= nullptr);
};

class tool {
public:
    // 将文本按行进行一分为二
    static std::tuple<std::string, std::string> splitFromLine(
        const std::string& data, int line);
};

#endif //MDTOOL2_SHARED_H