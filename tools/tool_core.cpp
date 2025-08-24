// tool/tool_core.cpp
// Created by zerox on 25-8-10.
//
#include "tool_core.h"

#include <re2/re2.h>
#include <re2/stringpiece.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "../other/path.h"
#include "../other/log.h"
#include "../other/coding.h"

// 统一的正则表达式模式
static const re2::RE2 kPattern(R"(```([ \t]*)([^ \t\n]*)([^\n]*)\n([\s\S]*?)([ \t]*)```)");

// 标准化换行符，将 \r\n 和 \r 都转换为 \n
std::string normalize_newlines(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\r') {
            if (i + 1 < str.size() && str[i + 1] == '\n') {
                ++i;
            }
            out.push_back('\n');
        } else {
            out.push_back(str[i]);
        }
    }
    return out;
}

// 读取文件内容的辅助函数 (支持多编码)
// 1. 读取原始二进制数据
// 2. 检测编码
// 3. 将内容转换为 UTF-8
// 4. 返回 UTF-8 内容和原始编码
bool read_file_content(const fs::path& path, std::string& utf8_content, std::string& original_encoding) {
    // 1. 读取原始二进制数据
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("无法打开文件: " + out_fs_path(path));
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string raw_content = buffer.str();

    if (raw_content.empty()) {
        utf8_content = "";
        original_encoding = "UTF-8"; // 对于空文件，假定为UTF-8
        return true;
    }

    // 2. 检测文件编码
    auto detected_encoding_res = encoding::detectFile(path);
    if (!detected_encoding_res || detected_encoding_res == std::string("unknown") ||  detected_encoding_res == std::string("empty")) {
        LOG_ERROR("文件编码检测失败: " + out_fs_path(path) +  " - " + encoding::errorToString(detected_encoding_res.error()));
        return false;
    }
    original_encoding = *detected_encoding_res;

    // 3. 将内容转换为 UTF-8
    auto to_utf8_res = encoding::toUTF8(raw_content, original_encoding);
    if (!to_utf8_res) {
        LOG_ERROR("无法将文件内容转换为UTF-8: " + out_fs_path(path) + " - " + encoding::errorToString(to_utf8_res.error()));
        return false;
    }
    utf8_content = *to_utf8_res;

    // 4. 标准化换行符 (在UTF-8内容上操作)
    utf8_content = normalize_newlines(utf8_content);
    return true;
}

// 写入文件内容的辅助函数 (支持多编码)
// 1. 接收 UTF-8 内容和目标编码
// 2. 将内容从 UTF-8 转换到目标编码
// 3. 将转换后的二进制数据写入文件
bool write_file_content(const fs::path& path, const std::string& utf8_content, const std::string& target_encoding) {
    std::string output_content;

    // 如果目标编码就是UTF-8 (或其不区分大小写的变体)，则无需转换
    std::string upper_target_encoding = target_encoding;
    std::ranges::transform(upper_target_encoding, upper_target_encoding.begin(), ::toupper);

    if (upper_target_encoding == "UTF-8" || upper_target_encoding == "UTF8") {
        output_content = utf8_content;
    } else {
        // 将UTF-8内容转换回原始编码
        auto convert_res = encoding::convertString(utf8_content, "UTF-8", target_encoding);
        if (!convert_res) {
            LOG_ERROR("无法将内容转换回原始编码 (" + target_encoding + "): " + out_fs_path(path) + " - " + encoding::errorToString(convert_res.error()));
            return false;
        }
        output_content = *convert_res;
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("无法写入文件: " + out_fs_path(path));
        return false;
    }

    file << output_content;
    return file.good();
}

// 验证文件的辅助函数
bool validate_file(const fs::path& path) {
    if (!fs::exists(path)) {
        LOG_ERROR("文件不存在: " + out_fs_path(path));
        return false;
    }
    if (!fs::is_regular_file(path)) {
        LOG_ERROR("不是一个有效的文件: " + out_fs_path(path));
        return false;
    }
    return true;
}

// 为单个md文档中的所有代码块添加语言（如果没有语言标记）
bool addl_(const fs::path& path, const std::string& language) {
    // 参数验证
    if (language.empty()) {
        LOG_ERROR("语言标识不能为空");
        return false;
    }

    // 验证文件
    if (!validate_file(path)) {
        return false;
    }

    try {
        // 读取文件内容
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        bool has_modification = false;

        // 使用RE2进行匹配
        re2::StringPiece input(content);
        re2::StringPiece leading_space, lang, rest_of_line, code_content, trailing_space;
        size_t last_end = 0;

        while (RE2::FindAndConsume(&input, kPattern, &leading_space, &lang, &rest_of_line, &code_content, &trailing_space)) {
            // 计算当前匹配的位置
            size_t match_start = content.size() - input.size() -
                                (3 + leading_space.size() + lang.size() + rest_of_line.size() + 1 +
                                 code_content.size() + trailing_space.size() + 3);

            // 添加代码块之前的内容
            result += content.substr(last_end, match_start - last_end);

            // 如果没有语言标识，添加语言标记
            if (lang.empty()) {
                result += "```";
                result.append(leading_space.data(), leading_space.size());
                result += language;
                result.append(rest_of_line.data(), rest_of_line.size());
                result += "\n";
                result.append(code_content.data(), code_content.size());
                result.append(trailing_space.data(), trailing_space.size());
                result += "```";
                has_modification = true;
            } else {
                // 保持原样
                result += "```";
                result.append(leading_space.data(), leading_space.size());
                result.append(lang.data(), lang.size());
                result.append(rest_of_line.data(), rest_of_line.size());
                result += "\n";
                result.append(code_content.data(), code_content.size());
                result.append(trailing_space.data(), trailing_space.size());
                result += "```";
            }

            last_end = content.size() - input.size();
        }

        // 添加最后一个代码块之后的内容
        result += content.substr(last_end);

        // 检查是否有修改
        if (!has_modification) {
            // 没有找到需要修改的代码块，但这不算错误
            return false;
        }

        // 写回文件
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("成功为文件添加语言标记: " + out_fs_path(path));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(std::string("处理文件时发生错误: ") + e.what());
        return false;
    }
}

// 将单个md文档中的所有代码块的语言更新（无论是否有语言标记）
bool updl_(const fs::path& path, const std::string& language) {
    // 参数验证
    if (language.empty()) {
        LOG_ERROR("语言标识不能为空");
        return false;
    }

    // 验证文件
    if (!validate_file(path)) {
        return false;
    }

    try {
        // 读取文件内容
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        bool has_modification = false;

        // 使用RE2进行匹配
        re2::StringPiece input(content);
        re2::StringPiece leading_space, lang, rest_of_line, code_content, trailing_space;
        size_t last_end = 0;

        while (RE2::FindAndConsume(&input, kPattern, &leading_space, &lang, &rest_of_line, &code_content, &trailing_space)) {
            // 计算当前匹配的位置
            size_t match_start = content.size() - input.size() -
                                (3 + leading_space.size() + lang.size() + rest_of_line.size() + 1 +
                                 code_content.size() + trailing_space.size() + 3);

            // 添加代码块之前的内容
            result += content.substr(last_end, match_start - last_end);

            // 统一更新语言标记
            result += "```";
            result.append(leading_space.data(), leading_space.size());
            result += language;
            result.append(rest_of_line.data(), rest_of_line.size());
            result += "\n";
            result.append(code_content.data(), code_content.size());
            result.append(trailing_space.data(), trailing_space.size());
            result += "```";
            has_modification = true;

            last_end = content.size() - input.size();
        }

        // 添加最后一个代码块之后的内容
        result += content.substr(last_end);

        // 检查是否有修改
        if (!has_modification) {
            // 没有找到代码块，但这不算错误
            return false;
        }

        // 写回文件
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("成功更新文件的语言标记: " + out_fs_path(path));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(std::string("处理文件时发生错误: ") + e.what());
        return false;
    }
}

// 添加一个函数来移除所有语言标记
bool rmvl_(const fs::path& path) {
    // 验证文件
    if (!validate_file(path)) {
        return false;
    }

    try {
        // 读取文件内容
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        bool has_modification = false;

        // 使用RE2进行匹配
        re2::StringPiece input(content);
        re2::StringPiece leading_space, lang, rest_of_line, code_content, trailing_space;
        size_t last_end = 0;

        while (RE2::FindAndConsume(&input, kPattern, &leading_space, &lang, &rest_of_line, &code_content, &trailing_space)) {
            // 计算当前匹配的位置
            size_t match_start = content.size() - input.size() -
                                (3 + leading_space.size() + lang.size() + rest_of_line.size() + 1 +
                                 code_content.size() + trailing_space.size() + 3);

            // 添加代码块之前的内容
            result += content.substr(last_end, match_start - last_end);

            // 如果有语言标识，移除它
            if (!lang.empty()) {
                result += "```";
                result.append(leading_space.data(), leading_space.size());
                result += "\n";
                result.append(code_content.data(), code_content.size());
                result.append(trailing_space.data(), trailing_space.size());
                result += "```";
                has_modification = true;
            } else {
                // 保持原样
                result += "```";
                result.append(leading_space.data(), leading_space.size());
                result.append(lang.data(), lang.size());
                result.append(rest_of_line.data(), rest_of_line.size());
                result += "\n";
                result.append(code_content.data(), code_content.size());
                result.append(trailing_space.data(), trailing_space.size());
                result += "```";
            }

            last_end = content.size() - input.size();
        }

        // 添加最后一个代码块之后的内容
        result += content.substr(last_end);

        // 检查是否有修改
        if (!has_modification) {
            return false;
        }

        // 写回文件
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("成功移除文件的语言标记: " + out_fs_path(path));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(std::string("处理文件时发生错误: ") + e.what());
        return false;
    }
}

// 对单个md文档的所有代码块的开头删除line行（为0则不删除，超过代码块范围将清空代码块）
bool delcl_(const fs::path& path, const unsigned int& line) {
    // 如果line为0，不需要处理
    if (line == 0) {
        return false;
    }

    // 验证文件
    if (!validate_file(path)) {
        return false;
    }

    try {
        // 读取文件内容
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        bool has_modification = false;

        // 使用RE2进行匹配
        re2::StringPiece input(content);
        re2::StringPiece leading_space, lang, rest_of_line, code_content, trailing_space;
        size_t last_end = 0;

        while (RE2::FindAndConsume(&input, kPattern, &leading_space, &lang, &rest_of_line, &code_content, &trailing_space)) {
            // 计算当前匹配的位置
            size_t match_start = content.size() - input.size() -
                                (3 + leading_space.size() + lang.size() + rest_of_line.size() + 1 +
                                 code_content.size() + trailing_space.size() + 3);

            // 添加代码块之前的内容
            result += content.substr(last_end, match_start - last_end);

            // 处理代码块内容：删除开头的line行
            std::string code_str(code_content.data(), code_content.size());
            std::string processed_content;

            // 将代码内容按行分割
            std::istringstream iss(code_str);
            std::vector<std::string> lines;
            std::string current_line;

            // 读取所有行
            while (std::getline(iss, current_line)) {
                lines.emplace_back(std::move(current_line));
            }

            // 如果原内容以换行符结尾但getline不会包含它，需要特殊处理
            if (!code_str.empty() && code_str.back() == '\n' &&
                (lines.empty() || !lines.back().empty())) {
                lines.emplace_back("");
            }

            // 删除开头的line行
            if (line < lines.size()) {
                // 从第line行开始保留
                for (size_t i = line; i < lines.size(); ++i) {
                    if (i > line) {
                        processed_content += "\n";
                    }
                    processed_content += lines[i];
                }

                // 如果原内容最后有换行符，保持一致
                if (!code_str.empty() && code_str.back() == '\n' &&
                    !processed_content.empty()) {
                    processed_content += "\n";
                }
            }
            // 如果line >= 行数，代码块内容将被清空

            // 检查是否有修改
            if (processed_content != code_str) {
                has_modification = true;
            }

            // 重新组装代码块
            result += "```";
            result.append(leading_space.data(), leading_space.size());
            result.append(lang.data(), lang.size());
            result.append(rest_of_line.data(), rest_of_line.size());
            result += "\n";
            result += processed_content;
            result.append(trailing_space.data(), trailing_space.size());
            result += "```";

            last_end = content.size() - input.size();
        }

        // 添加最后一个代码块之后的内容
        result += content.substr(last_end);

        // 如果没有修改，返回false
        if (!has_modification) {
            return false;
        }

        // 写回文件
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("成功删除文件代码块的前 " + std::to_string(line) + " 行: " + out_fs_path(path));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(std::string("处理文件时发生错误: ") + e.what());
        return false;
    }
}