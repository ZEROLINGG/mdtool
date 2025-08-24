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
 * @brief �ṩ�����ڼ��ʹ����ı������ʵ�ú���.
 */
namespace encoding {

    // �������Ͷ���
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

    // �������
    template<typename T>
    using Result = std::expected<T, EncodingError>;

    // RAII ��װ��
    class UchardetDetector {
    public:
        UchardetDetector();
        ~UchardetDetector();

        // ���ÿ����������ƶ�
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

        // ���ÿ����������ƶ�
        IconvConverter(const IconvConverter&) = delete;
        IconvConverter& operator=(const IconvConverter&) = delete;
        IconvConverter(IconvConverter&& other) noexcept;
        IconvConverter& operator=(IconvConverter&& other) noexcept;

        Result<std::string> convert(std::string_view input);

    private:
        iconv_t cd_;
    };

    /**
     * @brief ����ļ����ı�����
     * @param filePath �ļ���·��
     * @return ��⵽�ı������ƻ������Ϣ
     */
    Result<std::string> detectFile(const std::filesystem::path& filePath);

    /**
     * @brief ����ַ������ı�����
     * @param str Ҫ�����ַ���
     * @return ��⵽�ı������ƻ������Ϣ
     */
    Result<std::string> detectString(std::string_view str);

    /**
     * @brief ���ַ�����һ�ֱ���ת��Ϊ��һ�ֱ���
     * @param input �����ַ���
     * @param fromEncoding Դ����
     * @param toEncoding Ŀ�����
     * @return ת������ַ����������Ϣ
     */
    Result<std::string> convertString(std::string_view input,
                                    std::string_view fromEncoding,
                                    std::string_view toEncoding);

    /**
     * @brief ת���ļ�����
     * @param inputPath �����ļ�·��
     * @param outputPath ����ļ�·��
     * @param fromEncoding Դ���루���Ϊ�գ����Զ���⣩
     * @param toEncoding Ŀ�����
     * @return �ɹ��������Ϣ
     */
    Result<void> convertFile(const std::filesystem::path& inputPath,
                           const std::filesystem::path& outputPath,
                           std::string_view fromEncoding = "",
                           std::string_view toEncoding = "UTF-8");

    /**
     * @brief ���ַ���ת��ΪUTF-8����
     * @param input �����ַ���
     * @param fromEncoding Դ���루���Ϊ�գ����Զ���⣩
     * @return UTF-8������ַ����������Ϣ
     */
    Result<std::string> toUTF8(std::string_view input,
                             std::string_view fromEncoding = "");

    /**
     * @brief ����ת���ļ�����
     * @param inputDir ����Ŀ¼
     * @param outputDir ���Ŀ¼
     * @param fromEncoding Դ���루�����Զ���⣩
     * @param toEncoding Ŀ�����
     * @param recursive �Ƿ�ݹ鴦����Ŀ¼
     * @return ����ɹ����ļ������������Ϣ
     */
    Result<size_t> convertDirectory(const std::filesystem::path &inputDir,
                                    const std::filesystem::path &outputDir,
                                    std::string_view fromEncoding = "",
                                    std::string_view toEncoding = "UTF-8",
                                    bool recursive = false);

    // ��������
    std::string errorToString(EncodingError error);
    bool isValidEncoding(std::string_view encoding);
    std::vector<std::string> getSupportedEncodings();

    // ��ȡ����̨���룬ʧ�ܷ��ؿ��ַ���
    std::string getConsoleEncoding();

} // namespace encoding


#endif //CODING_H