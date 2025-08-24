// other/coding.h
// Created by zerox on 25-8-11.
//

#ifndef CODING_H
#define CODING_H

#include <filesystem>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <string_view>
#include <expected>
#include <uchardet/uchardet.h>
#include <iconv.h>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <locale.h>
#include <langinfo.h>
#endif

/**
 * @namespace encoding
 * @brief 提供了用于检测和处理文本编码的实用函数.
 */
namespace encoding {

    // 错误类型定义
    enum class EncodingError {
        FileNotFound,
        NotRegularFile,
        CannotOpenFile,
        CannotReadFile,
        EmptyInput,
        DetectorCreationFailed,
        DetectionFailed,
        ConversionFailed,
        InvalidInput
    };

    // 结果类型
    template<typename T>
    using Result = std::expected<T, EncodingError>;

    // RAII 封装类
    class UchardetDetector {
    public:
        UchardetDetector();
        ~UchardetDetector();

        // 禁用拷贝，允许移动
        UchardetDetector(const UchardetDetector&) = delete;
        UchardetDetector& operator=(const UchardetDetector&) = delete;
        UchardetDetector(UchardetDetector&& other) noexcept;
        UchardetDetector& operator=(UchardetDetector&& other) noexcept;

        Result<std::string> detect(std::string_view data);
        void reset();

    private:
        uchardet_t detector_;
    };

    class IconvConverter {
    public:
        IconvConverter(std::string_view from, std::string_view to);
        ~IconvConverter();

        // 禁用拷贝，允许移动
        IconvConverter(const IconvConverter&) = delete;
        IconvConverter& operator=(const IconvConverter&) = delete;
        IconvConverter(IconvConverter&& other) noexcept;
        IconvConverter& operator=(IconvConverter&& other) noexcept;

        Result<std::string> convert(std::string_view input);

    private:
        iconv_t cd_;
    };

    /**
     * @brief 检测文件的文本编码
     * @param filePath 文件的路径
     * @return 检测到的编码名称或错误信息
     */
    Result<std::string> detectFile(const std::filesystem::path& filePath);

    /**
     * @brief 检测字符串的文本编码
     * @param str 要检测的字符串
     * @return 检测到的编码名称或错误信息
     */
    Result<std::string> detectString(std::string_view str);

    /**
     * @brief 将字符串从一种编码转换为另一种编码
     * @param input 输入字符串
     * @param fromEncoding 源编码
     * @param toEncoding 目标编码
     * @return 转换后的字符串或错误信息
     */
    Result<std::string> convertString(std::string_view input,
                                    std::string_view fromEncoding,
                                    std::string_view toEncoding);

    /**
     * @brief 转换文件编码
     * @param inputPath 输入文件路径
     * @param outputPath 输出文件路径
     * @param fromEncoding 源编码（如果为空，则自动检测）
     * @param toEncoding 目标编码
     * @return 成功或错误信息
     */
    Result<void> convertFile(const std::filesystem::path& inputPath,
                           const std::filesystem::path& outputPath,
                           std::string_view fromEncoding = "",
                           std::string_view toEncoding = "UTF-8");

    /**
     * @brief 将字符串转换为UTF-8编码
     * @param input 输入字符串
     * @param fromEncoding 源编码（如果为空，则自动检测）
     * @return UTF-8编码的字符串或错误信息
     */
    Result<std::string> toUTF8(std::string_view input,
                             std::string_view fromEncoding = "");

    /**
     * @brief 批量转换文件编码
     * @param inputDir 输入目录
     * @param outputDir 输出目录
     * @param fromEncoding 源编码（空则自动检测）
     * @param toEncoding 目标编码
     * @param recursive 是否递归处理子目录
     * @return 处理成功的文件数量或错误信息
     */
    Result<size_t> convertDirectory(const std::filesystem::path &inputDir,
                                    const std::filesystem::path &outputDir,
                                    std::string_view fromEncoding = "",
                                    std::string_view toEncoding = "UTF-8",
                                    bool recursive = false);

    // 便利函数
    std::string errorToString(EncodingError error);
    bool isValidEncoding(std::string_view encoding);
    std::vector<std::string> getSupportedEncodings();

    // 获取控制台编码，失败返回空字符串
    std::string getConsoleEncoding();

} // namespace encoding


#endif //CODING_H