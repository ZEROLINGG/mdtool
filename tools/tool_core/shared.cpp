#include "shared.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <optional>
#include <tuple>


// 标准化换行符，将 \r\n 和 \r 都转换为 \n
std::string normalize_newlines(const std::string& str) {
    std::string out;
    out.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\r') {
            if (i + 1 < str.size() && str[i + 1] == '\n') {
                ++i;  // 跳过 \n
            }
            out.push_back('\n');
        } else {
            out.push_back(str[i]);
        }
    }

    return out;
}

std::optional<std::string> encoding::detect_file_charset(const char *filename) {
    if (!filename) {
        logs::print("文件名为空", LOG_TYPE::Error);
        return std::nullopt;
    }

    // 使用 RAII 管理文件
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        logs::print("打开文件失败: " + std::string(filename), LOG_TYPE::Error);
        return std::nullopt;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    const std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 创建检测器 (使用 RAII 智能指针)
    UchardetPtr ud(uchardet_new());
    if (!ud) {
        logs::print("创建字符集检测器失败", LOG_TYPE::Error);
        return std::nullopt;
    }

    // 分块读取文件并检测
    char buffer[BUFFER_SIZE];
    size_t total_read = 0;

    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
        const std::streamsize len = file.gcount();

        if (uchardet_handle_data(ud.get(), buffer, static_cast<size_t>(len)) != 0) {
            logs::print("处理数据失败", LOG_TYPE::Error);
            return std::nullopt;
        }

        total_read += static_cast<size_t>(len);

        // 如果已读取足够数据，提前结束
        if (total_read >= MAX_DETECTION_SIZE) {
            if (file_size > MAX_DETECTION_SIZE) {
                logs::print(
                    "文件较大(" + std::to_string(file_size) +
                    " 字节), 仅使用前 " + std::to_string(total_read) +
                    " 字节进行字符集检测",
                    LOG_TYPE::Info
                );
            }
            break;
        }
    }

    // 完成检测
    uchardet_data_end(ud.get());

    // 获取结果
    const char *charset = uchardet_get_charset(ud.get());
    if (!charset || strlen(charset) == 0) {
        logs::print("无法检测字符集，可能是二进制文件或编码复杂", LOG_TYPE::Error);
        return std::nullopt;
    }

    return std::string(charset);
}

std::optional<std::string> encoding::readToUtf8(const char *filename, std::string charset)
{
    if (!filename) {
        logs::print("文件名为空", LOG_TYPE::Error);
        return std::nullopt;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        logs::print("打开文件失败: " + std::string(filename), LOG_TYPE::Error);
        return std::nullopt;
    }

    // 读取所有内容
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size < 0) {
        logs::print("文件大小读取失败: " + std::string(filename), LOG_TYPE::Error);
        return std::nullopt;
    }

    std::vector<char> input(static_cast<size_t>(file_size));
    if (!file.read(input.data(), file_size)) {
        logs::print("读取文件内容失败", LOG_TYPE::Error);
        return std::nullopt;
    }

    // UTF-8 带 BOM → 去除 BOM，直接返回
    if (file_size >= 3 &&
        static_cast<unsigned char>(input[0]) == 0xEF &&
        static_cast<unsigned char>(input[1]) == 0xBB &&
        static_cast<unsigned char>(input[2]) == 0xBF)
    {
        // 直接从 BOM 后面开始构造 std::string
        std::string out(input.data() + 3, input.size() - 3);
        return normalize_newlines(out);
    }

    // UTF-8 无 BOM 直接返回
    if (charset == "UTF-8" || charset == "UTF8") {
        // 直接构造 std::string
        std::string out(input.data(), input.size());
        return normalize_newlines(out);
    }

    // 初始化 iconv：目标 UTF-8
    iconv_t cd_raw = iconv_open("UTF-8", charset.c_str());
    if (cd_raw == reinterpret_cast<iconv_t>(-1)) {
        logs::print("无法创建编码转换器: " + charset + " -> UTF-8", LOG_TYPE::Error);
        return std::nullopt;
    }
    IconvPtr cd(cd_raw);

    // 转换缓冲区，动态扩展
    size_t in_left  = input.size();
    char* in_ptr    = input.data();

    std::vector<char> output;
    output.reserve(input.size() * 2);

    constexpr size_t CHUNK = 65536;
    std::vector<char> temp(CHUNK);

    while (true) {
        char* out_ptr = temp.data();
        size_t out_left = CHUNK;

        size_t res = iconv(cd.get(), &in_ptr, &in_left, &out_ptr, &out_left);

        size_t produced = CHUNK - out_left;
        output.insert(output.end(), temp.data(), temp.data() + produced);

        if (res != static_cast<size_t>(-1))
            break;

        if (errno == E2BIG) {
            continue;
        }

        logs::print("编码转换失败: " + std::string(strerror(errno)), LOG_TYPE::Error);
        return std::nullopt;
    }

    // 构造 UTF-8 输出 (std::string)
    std::string result(output.data(), output.size());
    return normalize_newlines(result);
}

std::optional<std::string> encoding::readToUtf8(const char *filename)
{
    const auto charset_opt = detect_file_charset(filename);
    if (!charset_opt) {
        logs::print("无法检测文件编码", LOG_TYPE::Error);
        return std::nullopt;
    }

    return readToUtf8(filename, *charset_opt);
}

bool encoding::saveUtf8ToFile(const char *filename, const std::string& data,
                               const std::string& charset) {
    if (!filename) {
        logs::print("文件名为空", LOG_TYPE::Error);
        return false;
    }

    auto chrst = charset.c_str();

    // 如果目标编码是 UTF-8 或未指定，直接写入
    if (!chrst || strlen(chrst) == 0 || strcmp(chrst, "UTF-8") == 0) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            logs::print("打开文件失败: " + std::string(filename), LOG_TYPE::Error);
            return false;
        }

        // data.c_str() 返回 const char*，不再需要 to_char
        file.write(data.c_str(), static_cast<std::streamsize>(data.size()));

        if (!file.good()) {
            logs::print("写入文件失败: " + std::string(filename), LOG_TYPE::Error);
            return false;
        }

        return true;
    }

    // 初始化 iconv 转换器 (UTF-8 -> 目标编码)
    iconv_t cd_raw = iconv_open(chrst, "UTF-8");
    if (cd_raw == reinterpret_cast<iconv_t>(-1)) {
        logs::print("无法创建编码转换器: UTF-8 -> " + std::string(chrst),
                   LOG_TYPE::Error);
        return false;
    }

    // 使用 RAII 管理 iconv
    IconvPtr cd(cd_raw);

    // 准备输入数据
    size_t input_left = data.size();
    // data.c_str() 返回 const char*
    // iconv 需要 char**，所以我们进行 const_cast (这是 iconv 的常见用法)
    char* input_ptr = const_cast<char*>(data.c_str());

    // 准备输出缓冲区
    size_t output_size = data.size() * 4;
    std::vector<char> output_buffer(output_size);
    size_t output_left = output_size;
    char* output_ptr = output_buffer.data();

    // 执行转换
    size_t result = iconv(cd_raw, &input_ptr, &input_left, &output_ptr, &output_left);

    if (result == static_cast<size_t>(-1)) {
        logs::print("编码转换失败: UTF-8 -> " + std::string(chrst) +
                   ": " + std::string(strerror(errno)), LOG_TYPE::Error);
        return false;
    }

    // 计算转换后的数据大小
    size_t converted_size = output_size - output_left;

    // 写入文件
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        logs::print("打开文件失败: " + std::string(filename), LOG_TYPE::Error);
        return false;
    }

    file.write(output_buffer.data(), static_cast<std::streamsize>(converted_size));

    if (!file.good()) {
        logs::print("写入文件失败: " + std::string(filename), LOG_TYPE::Error);
        return false;
    }

    return true;
}


std::tuple<std::string, std::string> tool::splitFromLine(
    const std::string& data, const int line)
{
    // line == 0：不分割
    if (data.empty() || line == 0) {
        return {data, ""};
    }

    // 收集每一行的起始位置
    std::vector<size_t> line_positions;
    line_positions.reserve(128);
    line_positions.push_back(0);  // 第一行起点

    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == '\n') {
            if (i + 1 < data.size()) {
                line_positions.push_back(i + 1);
            }
        }
    }

    // 确保最后一行被统计
    if (line_positions.back() != data.size()) {
        line_positions.push_back(data.size());
    }

    const int total_lines = static_cast<int>(line_positions.size());
    size_t split_pos = 0;

    // ------------------------------------------------------------
    // line > 0 ：目标行属于第二部分
    // ------------------------------------------------------------
    if (line > 0) {
        if (line > total_lines) {
            // 行超范围 → 完全放左边
            return {data, ""};
        }

        // line 行的起始位置进入第二部分
        split_pos = line_positions[line - 1];
    }

    // ------------------------------------------------------------
    // line < 0 ：目标行属于第一部分
    // ------------------------------------------------------------
    else {
        int from_end = -line;

        if (from_end > total_lines) {
            // 倒数行超范围 → 全部放右边
            return {"", data};
        }

        // 倒数第 from_end 行
        int target_line_index = total_lines - from_end;

        // 该行应该属于第一部分 → 第二部分从下一行开始
        if (target_line_index + 1 < total_lines) {
            split_pos = line_positions[target_line_index + 1];
        } else {
            // 最后一行被划入第一部分 → 第二部分为空
            split_pos = data.size();
        }
    }

    // 执行分割
    std::string first_part  = data.substr(0, split_pos);
    std::string second_part = data.substr(split_pos);

    return {first_part, second_part};
}
