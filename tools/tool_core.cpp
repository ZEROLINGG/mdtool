// tool/tool_core.cpp
// Created by zerox on 25-8-10.
//
#include "tool_core.h"
#include <regex>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "../other/path.h"
#include "../other/log.h"
#include "../other/coding.h"

// ͳһ��������ʽģʽ
const std::regex pattern(R"(```([ \t]*)([^ \t\n]*)([^\n]*)\n([\s\S]*?)([ \t]*)```)");

// ��׼�����з����� \r\n �� \r ��ת��Ϊ \n
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

// ��ȡ�ļ����ݵĸ������� (���ع���֧�ֶ����)
// 1. ��ȡԭʼ����������
// 2. ������
// 3. ������ת��Ϊ UTF-8
// 4. ���� UTF-8 ���ݺ�ԭʼ����
bool read_file_content(const fs::path& path, std::string& utf8_content, std::string& original_encoding) {
    // 1. ��ȡԭʼ����������
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("�޷����ļ�: " + out_fs_path(path));
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string raw_content = buffer.str();

    if (raw_content.empty()) {
        utf8_content = "";
        original_encoding = "UTF-8"; // ���ڿ��ļ����ٶ�ΪUTF-8
        return true;
    }

    // 2. ����ļ�����
    auto detected_encoding_res = encoding::detectFile(path);
    if (!detected_encoding_res || detected_encoding_res == std::string("unknown") ||  detected_encoding_res == std::string("empty")) {
        LOG_ERROR("�ļ�������ʧ��: " + out_fs_path(path) +  " - " + encoding::errorToString(detected_encoding_res.error()));
        return false;
    }
    original_encoding = *detected_encoding_res;

    // 3. ������ת��Ϊ UTF-8
    auto to_utf8_res = encoding::toUTF8(raw_content, original_encoding);
    if (!to_utf8_res) {
        LOG_ERROR("�޷����ļ�����ת��ΪUTF-8: " + out_fs_path(path) + " - " + encoding::errorToString(to_utf8_res.error()));
        return false;
    }
    utf8_content = *to_utf8_res;

    // 4. ��׼�����з� (��UTF-8�����ϲ���)
    utf8_content = normalize_newlines(utf8_content);
    return true;
}

// д���ļ����ݵĸ������� (���ع���֧�ֶ����)
// 1. ���� UTF-8 ���ݺ�Ŀ�����
// 2. �����ݴ� UTF-8 ת����Ŀ�����
// 3. ��ת����Ķ���������д���ļ�
bool write_file_content(const fs::path& path, const std::string& utf8_content, const std::string& target_encoding) {
    std::string output_content;

    // ���Ŀ��������UTF-8 (���䲻���ִ�Сд�ı���)��������ת��
    std::string upper_target_encoding = target_encoding;
    std::ranges::transform(upper_target_encoding, upper_target_encoding.begin(), ::toupper);

    if (upper_target_encoding == "UTF-8" || upper_target_encoding == "UTF8") {
        output_content = utf8_content;
    } else {
        // ��UTF-8����ת����ԭʼ����
        auto convert_res = encoding::convertString(utf8_content, "UTF-8", target_encoding);
        if (!convert_res) {
            LOG_ERROR("�޷�������ת����ԭʼ���� (" + target_encoding + "): " + out_fs_path(path) + " - " + encoding::errorToString(convert_res.error()));
            return false;
        }
        output_content = *convert_res;
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("�޷�д���ļ�: " + out_fs_path(path));
        return false;
    }

    file << output_content;
    return file.good();
}

// ��֤�ļ��ĸ�������
bool validate_file(const fs::path& path) {
    if (!fs::exists(path)) {
        LOG_ERROR("�ļ�������: " + out_fs_path(path));
        return false;
    }
    if (!fs::is_regular_file(path)) {
        LOG_ERROR("����һ����Ч���ļ�: " + out_fs_path(path));
        return false;
    }
    return true;
}

// Ϊ����md�ĵ��е����д����������ԣ����û�����Ա�ǣ�
bool addl_(const fs::path& path, const std::string& language) {
    // ������֤
    if (language.empty()) {
        LOG_ERROR("���Ա�ʶ����Ϊ��");
        return false;
    }

    // ��֤�ļ�
    if (!validate_file(path)) {
        return false;
    }

    try {
        // ��ȡ�ļ�����
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        size_t last_pos = 0;
        bool has_modification = false;

        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            const std::smatch& match = *it;

            // ��Ӵ����֮ǰ������
            result += content.substr(last_pos, match.position() - last_pos);

            std::string leading_space = match[1].str();    // ```��Ŀհ�
            std::string lang = match[2].str();             // ���Ա�ʶ
            std::string rest_of_line = match[3].str();     // �е����ಿ��
            std::string code_content = match[4].str();     // ��������
            std::string trailing_space = match[5].str();   // ```ǰ�Ŀհ�

            // ���û�����Ա�ʶ��������Ա��
            if (lang.empty()) {
                result += "```";
                result += leading_space;
                result += language;
                result += rest_of_line;
                result += "\n";
                result += code_content;
                result += trailing_space;
                result += "```";
                has_modification = true;
            } else {
                // ����ԭ��
                result += match.str();
            }

            last_pos = match.position() + match.length();
        }

        // ������һ�������֮�������
        result += content.substr(last_pos);

        // ����Ƿ����޸�
        if (!has_modification) {
            // û���ҵ���Ҫ�޸ĵĴ���飬���ⲻ�����
            return false;
        }

        // д���ļ�
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("�ɹ�Ϊ�ļ�������Ա��: " + out_fs_path(path));
        return true;

    } catch (const std::regex_error& e) {
        LOG_ERROR(std::string("������ʽ����: ") + e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("�����ļ�ʱ��������: ") + e.what());
        return false;
    }
}

// ������md�ĵ��е����д��������Ը��£������Ƿ������Ա�ǣ�
bool updl_(const fs::path& path, const std::string& language) {
    // ������֤
    if (language.empty()) {
        LOG_ERROR("���Ա�ʶ����Ϊ��");
        return false;
    }

    // ��֤�ļ�
    if (!validate_file(path)) {
        return false;
    }

    try {
        // ��ȡ�ļ�����
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        size_t last_pos = 0;
        bool has_modification = false;

        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            const std::smatch& match = *it;

            // ��Ӵ����֮ǰ������
            result += content.substr(last_pos, match.position() - last_pos);

            std::string leading_space = match[1].str();    // ```��Ŀհ�
            std::string lang = match[2].str();             // ���Ա�ʶ
            std::string rest_of_line = match[3].str();     // �е����ಿ��
            std::string code_content = match[4].str();     // ��������
            std::string trailing_space = match[5].str();   // ```ǰ�Ŀհ�

            // ͳһ�������Ա��
            result += "```";
            result += leading_space;
            result += language;
            result += rest_of_line;
            result += "\n";
            result += code_content;
            result += trailing_space;
            result += "```";
            has_modification = true;

            last_pos = match.position() + match.length();
        }

        // ������һ�������֮�������
        result += content.substr(last_pos);

        // ����Ƿ����޸�
        if (!has_modification) {
            // û���ҵ�����飬���ⲻ�����
            return false;
        }

        // д���ļ�
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("�ɹ������ļ������Ա��: " + out_fs_path(path));
        return true;

    } catch (const std::regex_error& e) {
        LOG_ERROR(std::string("������ʽ����: ") + e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("�����ļ�ʱ��������: ") + e.what());
        return false;
    }
}

// ���һ���������Ƴ��������Ա��
bool rmvl_(const fs::path& path) {
    // ��֤�ļ�
    if (!validate_file(path)) {
        return false;
    }

    try {
        // ��ȡ�ļ�����
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        size_t last_pos = 0;
        bool has_modification = false;

        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            const std::smatch& match = *it;

            // ��Ӵ����֮ǰ������
            result += content.substr(last_pos, match.position() - last_pos);

            std::string leading_space = match[1].str();    // ```��Ŀհ�
            std::string lang = match[2].str();             // ���Ա�ʶ
            std::string rest_of_line = match[3].str();     // �е����ಿ��
            std::string code_content = match[4].str();     // ��������
            std::string trailing_space = match[5].str();   // ```ǰ�Ŀհ�

            // ��������Ա�ʶ���Ƴ���
            if (!lang.empty()) {
                result += "```";
                result += leading_space;
                result += "\n";
                result += code_content;
                result += trailing_space;
                result += "```";
                has_modification = true;
            } else {
                // ����ԭ��
                result += match.str();
            }

            last_pos = match.position() + match.length();
        }

        // ������һ�������֮�������
        result += content.substr(last_pos);

        // ����Ƿ����޸�
        if (!has_modification) {
            return false;
        }

        // д���ļ�
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("�ɹ��Ƴ��ļ������Ա��: " + out_fs_path(path));
        return true;

    } catch (const std::regex_error& e) {
        LOG_ERROR(std::string("������ʽ����: ") + e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("�����ļ�ʱ��������: ") + e.what());
        return false;
    }
}

// �Ե���md�ĵ������д����Ŀ�ͷɾ��line�У�Ϊ0��ɾ������������鷶Χ����մ���飩
bool delcl_(const fs::path& path, const unsigned int& line) {
    // ���lineΪ0������Ҫ����
    if (line == 0) {
        return false;
    }

    // ��֤�ļ�
    if (!validate_file(path)) {
        return false;
    }

    try {
        // ��ȡ�ļ�����
        std::string content;
        std::string original_encoding;
        if (!read_file_content(path, content, original_encoding)) {
            return false;
        }

        std::string result;
        size_t last_pos = 0;
        bool has_modification = false;

        std::sregex_iterator it(content.begin(), content.end(), pattern);

        for (std::sregex_iterator end; it != end; ++it) {
            const std::smatch& match = *it;

            // ��Ӵ����֮ǰ������
            result += content.substr(last_pos, match.position() - last_pos);

            std::string leading_space = match[1].str();    // ```��Ŀհ�
            std::string lang = match[2].str();             // ���Ա�ʶ
            std::string rest_of_line = match[3].str();     // �е����ಿ��
            std::string code_content = match[4].str();     // ��������
            std::string trailing_space = match[5].str();   // ```ǰ�Ŀհ�

            // �����������ݣ�ɾ����ͷ��line��
            std::string processed_content;

            // ���������ݰ��зָ�
            std::istringstream iss(code_content);
            std::vector<std::string> lines;
            std::string current_line;

            // ��ȡ������
            while (std::getline(iss, current_line)) {
                lines.emplace_back(std::move(current_line));
            }

            // ���ԭ�����Ի��з���β��getline�������������Ҫ���⴦��
            if (!code_content.empty() && code_content.back() == '\n' &&
                (lines.empty() || !lines.back().empty())) {
                lines.emplace_back("");
            }

            // ɾ����ͷ��line��
            if (line < lines.size()) {
                // �ӵ�line�п�ʼ����
                for (size_t i = line; i < lines.size(); ++i) {
                    if (i > line) {
                        processed_content += "\n";
                    }
                    processed_content += lines[i];
                }

                // ���ԭ��������л��з�������һ��
                if (!code_content.empty() && code_content.back() == '\n' &&
                    !processed_content.empty()) {
                    processed_content += "\n";
                }
            }
            // ���line >= ��������������ݽ������

            // ����Ƿ����޸�
            if (processed_content != code_content) {
                has_modification = true;
            }

            // ������װ�����
            result += "```";
            result += leading_space;
            result += lang;
            result += rest_of_line;
            result += "\n";
            result += processed_content;
            result += trailing_space;
            result += "```";

            last_pos = match.position() + match.length();
        }

        // ������һ�������֮�������
        result += content.substr(last_pos);

        // ���û���޸ģ�����false
        if (!has_modification) {
            return false;
        }

        // д���ļ�
        if (!write_file_content(path, result, original_encoding)) {
            return false;
        }

        LOG_INFO("�ɹ�ɾ���ļ�������ǰ " + std::to_string(line) + " ��: " + out_fs_path(path));
        return true;

    } catch (const std::regex_error& e) {
        LOG_ERROR(std::string("������ʽ����: ") + e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("�����ļ�ʱ��������: ") + e.what());
        return false;
    }
}