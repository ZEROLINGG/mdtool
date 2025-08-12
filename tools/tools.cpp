// tools/tools.cpp
// Created by zerox on 25-8-10.
// ʹ��c++  23
#include "tools.h"
#include "tool_core.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <functional> // ��Ҫ���� <functional>

#include "../other/path.h"
#include "../other/log.h"

namespace tools {
    namespace fs = std::filesystem;

    namespace { // ʹ�����������ռ������ظ��������ͳ���
        // constexpr double kDefaultPathScanTimeout = 1.5;
        // ��kDefaultPathScanTimeout�ƶ���Options.kDefaultPathScanTimeout����

        // �ж��Ƿ���md�ĵ�
        bool is_markdown(const fs::path& path) {
            auto ext = path.extension().string();
            std::ranges::transform(ext, ext.begin(),
                [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return ext == ".md" || ext == ".markdown";
        }

        // �ж�·������
        path_info detect_path_type(const fs::path& path, const double timeout) {
            std::error_code ec;

            // ���·���Ƿ���� (�򻯴�����)
            if (!fs::exists(path, ec) || ec) {
                return PATH_ERROR;
            }

            // ����Ƿ�Ϊ�ļ�
            if (fs::is_regular_file(path, ec)) {
                return ec ? PATH_ERROR : (is_markdown(path) ? FILE_MD : FILE_OTHER);
            }

            // ����Ƿ�ΪĿ¼
            if (fs::is_directory(path, ec)) {
                if (ec) return PATH_ERROR;

                const auto start_time = std::chrono::steady_clock::now();
                try {
                    bool found_markdown = false;
                    for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
                        auto current_time = std::chrono::steady_clock::now();

                        if (const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count(); static_cast<double>(duration) > timeout * 1000) {
                            return VERY_BIG;
    }

                        if (entry.is_regular_file(ec) && !ec && is_markdown(entry.path())) {
                            found_markdown = true;
                            // �Ż������ֻ��Ҫ֪����û��md�ļ�������������break����Ϊ�˼�鳬ʱ����Ҫ������
                            // ����ԭ�߼���
                        }
                    }
                    return found_markdown ? DIR_MD : DIR_OTHER;
                } catch (const fs::filesystem_error& e) {
                    LOG_ERROR("����Ŀ¼ʱ����: " + std::string(e.what()));
                    return PATH_ERROR;
                }
            }

            return PATH_ERROR; // ����δ֪����
        }

        // ����һ���������ͣ�����Ե����ļ����еĲ���
        using FileProcessor = std::function<bool(const fs::path&)>;

        // ͨ�õĴ�����
        void process_markdown_paths(const fs::path& target_path, const double timeout, const FileProcessor& processor) {
            switch (detect_path_type(target_path, timeout)) {
                case VERY_BIG:
                    LOG_WARN("Ŀ¼�������ʱ����ȷ��·���Ƿ���ȷ��");
                    LOG_WARN("�����Ҫ�����Ŀ¼���趨--timeout");
                    LOG_WARN("·��: [" + out_fs_path(target_path) + "]");
                    break;
                case PATH_ERROR:
                    LOG_ERROR("·�������ڻ��޷�����!");
                    LOG_ERROR("·��: [" + out_fs_path(target_path) + "]");
                    break;
                case FILE_OTHER:
                case DIR_OTHER:
                    LOG_ERROR("·����û���ҵ�Markdown�ļ�!");
                    LOG_ERROR("·��: [" + out_fs_path(target_path) + "]");
                    break;
                case FILE_MD:
                    LOG_INFO("������Markdown�ļ�: " + out_fs_path(target_path));
                    try {
                        if (processor(target_path)) {
                           LOG_INFO("������ɡ�");
                        } else {
                            LOG_INFO("δ�����޸ġ�");
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR("�����ļ�ʧ��: " + std::string(e.what()));
                    }
                    break;
                case DIR_MD:
                    LOG_INFO("����Ŀ¼�е�Markdown�ļ�: " + out_fs_path(target_path));
                    try {
                        int processed_count = 0;
                        for (const auto& entry : fs::recursive_directory_iterator(target_path, fs::directory_options::skip_permission_denied)) {
                            if (entry.is_regular_file() && is_markdown(entry.path())) {
                                // LOG_INFO("�����ļ�: " + out_fs_path(entry.path()));
                                try {
                                    if (processor(entry.path())) {
                                        processed_count++;
                                        LOG_INFO("�������: " + out_fs_path(entry.path()));
                                    } else {
                                        // LOG_INFO("δ�����޸�; " + out_fs_path(entry.path()));
                                    }
                                } catch (const std::exception& e) {
                                    LOG_ERROR("�����ļ�ʧ�� [" + out_fs_path(entry.path()) + "]: " + std::string(e.what()));
                                }
                            }
                        }
                        LOG_INFO("�������� " + std::to_string(processed_count) + " ��Markdown�ļ���");
                    } catch (const fs::filesystem_error& e) {
                        LOG_ERROR("����Ŀ¼ʱ����: " + std::string(e.what()));
                    }
                    break;
            }
        }
    } // end anonymous namespace


    void addl(const Options& Options) {
        if (Options.language.empty()) {
            LOG_WARN("��ʹ��-l �������");
            return;
        }
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return addl_(p, Options.language);
        });
    }

    void updl(const Options& Options) {
        if (Options.language.empty()) {
            LOG_WARN("��ʹ��-l �������");
            return;
        }
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return updl_(p, Options.language);
        });
    }

    void rmvl(const Options& Options) {
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return rmvl_(p);
        });
    }

    void delcl(const Options& Options) {
        if (Options.line == UINT_MAX) {
            LOG_WARN("ʹ��Ĭ��ֵ������մ���飡����");
            LOG_WARN("������ y ִ�в�����y/N��");
            std::string confirm;
            std::getline(std::cin, confirm);
            confirm.erase(0, confirm.find_first_not_of(" \t\r\n"));
            confirm.erase(confirm.find_last_not_of(" \t\r\n") + 1);
            if (confirm != "y" && confirm != "Y") {
                LOG_INFO("������ȡ����");
                return;
            }
        }
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return delcl_(p, Options.line);
        });
    }

} // namespace tools