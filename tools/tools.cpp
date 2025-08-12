// tools/tools.cpp
// Created by zerox on 25-8-10.
// 使用c++  23
#include "tools.h"
#include "tool_core.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <functional> // 需要包含 <functional>

#include "../other/path.h"
#include "../other/log.h"

namespace tools {
    namespace fs = std::filesystem;

    namespace { // 使用匿名命名空间来隐藏辅助函数和常量
        // constexpr double kDefaultPathScanTimeout = 1.5;
        // 将kDefaultPathScanTimeout移动到Options.kDefaultPathScanTimeout中了

        // 判断是否是md文档
        bool is_markdown(const fs::path& path) {
            auto ext = path.extension().string();
            std::ranges::transform(ext, ext.begin(),
                [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return ext == ".md" || ext == ".markdown";
        }

        // 判断路径类型
        path_info detect_path_type(const fs::path& path, const double timeout) {
            std::error_code ec;

            // 检查路径是否存在 (简化错误检查)
            if (!fs::exists(path, ec) || ec) {
                return PATH_ERROR;
            }

            // 检查是否为文件
            if (fs::is_regular_file(path, ec)) {
                return ec ? PATH_ERROR : (is_markdown(path) ? FILE_MD : FILE_OTHER);
            }

            // 检查是否为目录
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
                            // 优化：如果只需要知道有没有md文件，可以在这里break，但为了检查超时，需要继续。
                            // 保持原逻辑。
                        }
                    }
                    return found_markdown ? DIR_MD : DIR_OTHER;
                } catch (const fs::filesystem_error& e) {
                    LOG_ERROR("遍历目录时出错: " + std::string(e.what()));
                    return PATH_ERROR;
                }
            }

            return PATH_ERROR; // 其他未知类型
        }

        // 定义一个函数类型，代表对单个文件进行的操作
        using FileProcessor = std::function<bool(const fs::path&)>;

        // 通用的处理函数
        void process_markdown_paths(const fs::path& target_path, const double timeout, const FileProcessor& processor) {
            switch (detect_path_type(target_path, timeout)) {
                case VERY_BIG:
                    LOG_WARN("目录过大或处理超时，请确认路径是否正确。");
                    LOG_WARN("如果需要处理大目录请设定--timeout");
                    LOG_WARN("路径: [" + out_fs_path(target_path) + "]");
                    break;
                case PATH_ERROR:
                    LOG_ERROR("路径不存在或无法访问!");
                    LOG_ERROR("路径: [" + out_fs_path(target_path) + "]");
                    break;
                case FILE_OTHER:
                case DIR_OTHER:
                    LOG_ERROR("路径中没有找到Markdown文件!");
                    LOG_ERROR("路径: [" + out_fs_path(target_path) + "]");
                    break;
                case FILE_MD:
                    LOG_INFO("处理单个Markdown文件: " + out_fs_path(target_path));
                    try {
                        if (processor(target_path)) {
                           LOG_INFO("处理完成。");
                        } else {
                            LOG_INFO("未发生修改。");
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR("处理文件失败: " + std::string(e.what()));
                    }
                    break;
                case DIR_MD:
                    LOG_INFO("处理目录中的Markdown文件: " + out_fs_path(target_path));
                    try {
                        int processed_count = 0;
                        for (const auto& entry : fs::recursive_directory_iterator(target_path, fs::directory_options::skip_permission_denied)) {
                            if (entry.is_regular_file() && is_markdown(entry.path())) {
                                // LOG_INFO("处理文件: " + out_fs_path(entry.path()));
                                try {
                                    if (processor(entry.path())) {
                                        processed_count++;
                                        LOG_INFO("处理完成: " + out_fs_path(entry.path()));
                                    } else {
                                        // LOG_INFO("未发生修改; " + out_fs_path(entry.path()));
                                    }
                                } catch (const std::exception& e) {
                                    LOG_ERROR("处理文件失败 [" + out_fs_path(entry.path()) + "]: " + std::string(e.what()));
                                }
                            }
                        }
                        LOG_INFO("共处理了 " + std::to_string(processed_count) + " 个Markdown文件。");
                    } catch (const fs::filesystem_error& e) {
                        LOG_ERROR("遍历目录时出错: " + std::string(e.what()));
                    }
                    break;
            }
        }
    } // end anonymous namespace


    void addl(const Options& Options) {
        if (Options.language.empty()) {
            LOG_WARN("请使用-l 添加语言");
            return;
        }
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return addl_(p, Options.language);
        });
    }

    void updl(const Options& Options) {
        if (Options.language.empty()) {
            LOG_WARN("请使用-l 添加语言");
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
            LOG_WARN("使用默认值将会清空代码块！！！");
            LOG_WARN("请输入 y 执行操作（y/N）");
            std::string confirm;
            std::getline(std::cin, confirm);
            confirm.erase(0, confirm.find_first_not_of(" \t\r\n"));
            confirm.erase(confirm.find_last_not_of(" \t\r\n") + 1);
            if (confirm != "y" && confirm != "Y") {
                LOG_INFO("操作已取消。");
                return;
            }
        }
        process_markdown_paths(Options.path, Options.kDefaultPathScanTimeout, [&](const fs::path& p) {
            return delcl_(p, Options.line);
        });
    }

} // namespace tools