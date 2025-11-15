//
// Created by zerox on 2025/11/7.
//

#ifndef MDTOOL2_FINALFUNCTION_H
#define MDTOOL2_FINALFUNCTION_H

// ReSharper disable once CppUnusedIncludeDirective
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "./tools/tool_core/shared.h"

namespace fs = std::filesystem;

enum class LOG_TYPE {
    Info = 4,
    MainInfo = 3,
    Warn = 2,
    Error = 1,
};

inline auto useLog = LOG_TYPE::MainInfo;

namespace logs {
    using alog = std::tuple<LOG_TYPE, std::string>;

    // 单条日志输出
    inline void printLog(const alog& log) {
        const auto& [type, msg] = log;
        // 输出前缀
        std::string prefix;
        std::ostream* out = &std::cout;  // 默认输出到标准输出

        switch (type) {
            case LOG_TYPE::Info:
                prefix = "[INFO] ";
                break;
            case LOG_TYPE::MainInfo:
                prefix = "[MAIN] ";
                break;
            case LOG_TYPE::Warn:
                prefix = "[WARN] ";
                break;
            case LOG_TYPE::Error:
                prefix = "[ERRO] ";
                out = &std::cerr;
                break;
            default:
                prefix = "[LOG]  ";
                break;
        }

        (*out) << std::left << std::setw(8)
               << prefix << msg << std::endl;
    }

    inline void printLog(const alog& log, const LOG_TYPE useLog) {
        if (const auto& [type, msg] = log; static_cast<int>(useLog) <= static_cast<int>(type)) {
            printLog(log);
        }
    }

    // 批量日志输出
    inline void printLogs(const std::vector<alog>& logs, const LOG_TYPE useLog) {
        if (logs.empty()) {
            if (useLog == LOG_TYPE::Info) {
                std::cout << "没有生成任何日志" << std::endl;
            }
            return;
        }
        for (const auto& l : logs) {
            printLog(l, useLog);
        }
    }

    // 获取用户确认输入
    inline bool getUserConfirmation(const bool defaultYes) {
        std::string input;
        std::getline(std::cin, input);

        // 去除首尾空格
        const size_t start = input.find_first_not_of(" \t\r\n");
        const size_t end = input.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            input = input.substr(start, end - start + 1);
        }

        // 空输入使用默认值
        if (input.empty()) {
            return defaultYes;
        }

        // 转换为小写比较
        for (char& c : input) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        if (input == "y" || input == "yes") {
            return true;
        }
        if (input == "n" || input == "no") {
            return false;
        }

        // 无效输入使用默认值
        return defaultYes;
    }

    // 增强的打印函数,支持Warn日志的交互确认
    inline bool print(
        const std::string& msg,
        LOG_TYPE type = LOG_TYPE::Info,
        const std::optional<bool> nextFlag = std::nullopt,
        const std::string& tips = "是否继续操作"
    ) {
        // nextFlag表示在Warn日志打印后是否继续操作的默认选项
        // true表示 (Y/n), false表示 (y/N), nullopt表示不需要确认直接继续
        // tips在Warn日志后打印,如: [WARN] 当前文件过大！\n是否继续操作？(Y/n):

        // 检查是否需要打印该日志
        if (static_cast<int>(useLog) > static_cast<int>(type)) {
            return true;  // 日志级别不够,不打印但返回true继续执行
        }

        // 打印日志
        printLog(alog(type, msg));

        // 如果是Warn类型且提供了nextFlag,进行交互确认
        if (type == LOG_TYPE::Warn && nextFlag.has_value()) {
            const bool defaultYes = nextFlag.value();
            std::cout << tips;

            if (defaultYes) {
                std::cout << " (Y/n): ";
            } else {
                std::cout << " (y/N): ";
            }
            std::cout.flush();

            return getUserConfirmation(defaultYes);
        }

        if (type == LOG_TYPE::Error) {
            return false;
        }
        return true;
    }

    // 重载版本: 直接传入alog对象
    inline bool print(
        const alog& log,
        const std::optional<bool> nextFlag = std::nullopt,
        const std::string& tips = "是否继续操作"
    ) {
        const auto& [type, msg] = log;
        return print(msg, type, nextFlag, tips);
    }
}




// 传递给处理函数的统一结构体
struct InputOptions {
    std::filesystem::path path;
    std::string input;        // 操作所需要的输入内容
    std::string input_file;
    std::string path_str;     // 单个md文档路径,或包含md文档的文件夹路径
    std::string option;
    int start;        // 指定md文档中操作的起始位置
    int number = 0;           // 指定在对象中操作的位置或数量
    int log = 3;
    LOG_TYPE useLog = LOG_TYPE::Info;
    std::optional<bool> bakup = std::nullopt;
    double kDefaultPathScanTimeout = 1.5;
    std::vector<logs::alog> logs; // 命令行解析日志暂存
};

struct FinalFuncReturn {
    bool success = false;
    std::vector<logs::alog> logs;
};

using FinalFunc = FinalFuncReturn(*)(const InputOptions&);

#endif //MDTOOL2_FINALFUNCTION_H