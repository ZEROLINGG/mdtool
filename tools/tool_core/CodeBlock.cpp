//
// Created by zerox on 2025/11/10.
//

#include "CodeBlock.h"


const RE2 CodeBlock::codeBlockRegex(R"(```([ \t]*)([^ \t\n]*)([^\n]*)\n([\s\S]*?)([ \t]*)```)");


FinalFuncReturn CodeBlock::LanguageIdentifier::add(const fs::path& path, const std::string& language, const int& start) {
    FinalFuncReturn rt;
    auto filename = reinterpret_cast<const char *>(path.c_str());
    const auto charset = enc.detect_file_charset(filename);
    if (!charset) {
        rt.success = false;
        rt.logs = {logs::alog(LOG_TYPE::Error,"检测字符集遇到错误")};
        return rt;
    }
    const auto data = enc.readToUtf8(filename, *charset);
    if (!data) {
        rt.success = false;
        rt.logs = {logs::alog(LOG_TYPE::Error,"读取文件为utf8遇到错误")};
        return rt;
    }
    auto text = tool::splitFromLine(*data, start);

    std::string content = start > 0 ? std::get<1>(text) : std::get<0>(text);


    std::string result;
    bool has_modification = false;
    re2::StringPiece input(content);
    re2::StringPiece leading_space, lang, rest_of_line, code_content, trailing_space;
    size_t last_end = 0;

    while (RE2::FindAndConsume(&input, codeBlockRegex, &leading_space, &lang, &rest_of_line, &code_content, &trailing_space)) {
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

    if (has_modification) {
        // 添加最后一个代码块之后的内容
        result += content.substr(last_end);
        if (start > 0) {
            std::get<1>(text) = result;
        } else {
            std::get<0>(text) = result;
        }

        bool save = enc.saveUtf8ToFile(filename, std::get<0>(text) + std::get<1>(text), *charset);
        if (!save) {
            rt.success = false;
            rt.logs = {logs::alog(LOG_TYPE::Error, "保存失败：" + std::string(filename))};
            return rt;
        }
        rt.success = true;
        rt.logs = {logs::alog(LOG_TYPE::Info, "处理完成：" + std::string(filename))};

        return rt;
    }

    rt.success = true;
    rt.logs = {logs::alog(LOG_TYPE::Info, "未发生修改：" + std::string(filename))};
    return rt;
}
