// other/coding.cpp
// Created by zerox on 25-8-11.
//

#include "coding.h"
#include "log.h"
#include <algorithm>
#include <ranges>
#include <span>
#include <format>

namespace encoding {

// UchardetDetector ʵ��
UchardetDetector::UchardetDetector() : detector_(uchardet_new()) {}

UchardetDetector::~UchardetDetector() {
    if (detector_) {
        uchardet_delete(detector_);
    }
}

UchardetDetector::UchardetDetector(UchardetDetector&& other) noexcept
    : detector_(other.detector_) {
    other.detector_ = nullptr;
}

UchardetDetector& UchardetDetector::operator=(UchardetDetector&& other) noexcept {
    if (this != &other) {
        if (detector_) {
            uchardet_delete(detector_);
        }
        detector_ = other.detector_;
        other.detector_ = nullptr;
    }
    return *this;
}

Result<std::string> UchardetDetector::detect(std::string_view data) {
    if (!detector_) {
        return std::unexpected(EncodingError::DetectorCreationFailed);
    }

    if (data.empty()) {
        return std::unexpected(EncodingError::EmptyInput);
    }

    reset();

    if (uchardet_handle_data(detector_, data.data(), data.size()) != 0) {
        return std::unexpected(EncodingError::DetectionFailed);
    }

    uchardet_data_end(detector_);

    const char* charset = uchardet_get_charset(detector_);
    if (!charset || !*charset) {
        return std::string("unknown");
    }

    return std::string(charset);
}

void UchardetDetector::reset() {
    if (detector_) {
        uchardet_reset(detector_);
    }
}

// IconvConverter ʵ��
IconvConverter::IconvConverter(std::string_view from, std::string_view to)
    : cd_(iconv_open(std::string(to).c_str(), std::string(from).c_str())) {}

IconvConverter::~IconvConverter() {
    if (cd_ != (iconv_t)-1) {
        iconv_close(cd_);
    }
}

IconvConverter::IconvConverter(IconvConverter&& other) noexcept : cd_(other.cd_) {
    other.cd_ = (iconv_t)-1;
}

IconvConverter& IconvConverter::operator=(IconvConverter&& other) noexcept {
    if (this != &other) {
        if (cd_ != (iconv_t)-1) {
            iconv_close(cd_);
        }
        cd_ = other.cd_;
        other.cd_ = (iconv_t)-1;
    }
    return *this;
}

Result<std::string> IconvConverter::convert(std::string_view input) {
    if (cd_ == (iconv_t)-1) {
        return std::unexpected(EncodingError::ConversionFailed);
    }

    if (input.empty()) {
        return std::string{};
    }

    // Ԥ�������������С��ͨ��4���㹻��
    constexpr size_t BUFFER_MULTIPLIER = 4;
    size_t inBytesLeft = input.size();
    size_t outBytesLeft = inBytesLeft * BUFFER_MULTIPLIER;

    std::vector<char> outputBuffer(outBytesLeft);

    char* inBuf = const_cast<char*>(input.data());
    char* outBuf = outputBuffer.data();
    char* outBufStart = outBuf;

    size_t result = iconv(cd_, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
    if (result == static_cast<size_t>(-1)) {
        return std::unexpected(EncodingError::ConversionFailed);
    }

    size_t outputSize = outBuf - outBufStart;
    return std::string(outputBuffer.data(), outputSize);
}

// ��Ҫ���ܺ���ʵ��
Result<std::string> detectFile(const std::filesystem::path& filePath) {
    std::error_code ec;

    if (!std::filesystem::exists(filePath, ec)) {
        return std::unexpected(EncodingError::FileNotFound);
    }

    if (!std::filesystem::is_regular_file(filePath, ec)) {
        return std::unexpected(EncodingError::NotRegularFile);
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::unexpected(EncodingError::CannotOpenFile);
    }

    const auto size = file.tellg();
    if (size == 0) {
        return std::string("empty");
    }

    file.seekg(0, std::ios::beg);

    // ���ڴ��ļ���ֻ��ȡǰ64KB���м��
    constexpr size_t MAX_DETECTION_SIZE = 64 * 1024;
    const size_t readSize = std::min(static_cast<size_t>(size), MAX_DETECTION_SIZE);

    std::vector<char> buffer(readSize);
    if (!file.read(buffer.data(), readSize)) {
        return std::unexpected(EncodingError::CannotReadFile);
    }

    UchardetDetector detector;
    return detector.detect(std::string_view(buffer.data(), buffer.size()));
}

Result<std::string> detectString(std::string_view str) {
    if (str.empty()) {
        return std::string("empty");
    }

    UchardetDetector detector;
    return detector.detect(str);
}

Result<std::string> convertString(std::string_view input,
                                std::string_view fromEncoding,
                                std::string_view toEncoding) {
    if (input.empty()) {
        return std::string{};
    }

    // ���Դ�����Ŀ�������ͬ��ֱ�ӷ���
    if (fromEncoding == toEncoding) {
        return std::string(input);
    }

    IconvConverter converter(fromEncoding, toEncoding);
    return converter.convert(input);
}

Result<void> convertFile(const std::filesystem::path& inputPath,
                       const std::filesystem::path& outputPath,
                       std::string_view fromEncoding,
                       std::string_view toEncoding) {
    // ��֤�����ļ�
    std::error_code ec;
    if (!std::filesystem::exists(inputPath, ec)) {
        return std::unexpected(EncodingError::FileNotFound);
    }
    if (!std::filesystem::is_regular_file(inputPath, ec)) {
        return std::unexpected(EncodingError::NotRegularFile);
    }

    // ��ȡ�����ļ�
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        return std::unexpected(EncodingError::CannotOpenFile);
    }

    std::string content((std::istreambuf_iterator<char>(inputFile)),
                       std::istreambuf_iterator<char>());
    inputFile.close();

    // ȷ��Դ����
    std::string sourceEncoding(fromEncoding);
    if (sourceEncoding.empty()) {
        auto detectionResult = detectString(content);
        if (!detectionResult) {
            return std::unexpected(detectionResult.error());
        }
        sourceEncoding = *detectionResult;
        if (sourceEncoding == "unknown" || sourceEncoding == "empty") {
            return std::unexpected(EncodingError::DetectionFailed);
        }
        LOG_INFO(std::format("Detected encoding: {}", sourceEncoding));
    }

    // ת������
    auto conversionResult = convertString(content, sourceEncoding, toEncoding);
    if (!conversionResult) {
        return std::unexpected(conversionResult.error());
    }

    // �������Ŀ¼����������ڣ�
    const auto outputDir = outputPath.parent_path();
    if (!outputDir.empty() && !std::filesystem::exists(outputDir, ec)) {
        if (!std::filesystem::create_directories(outputDir, ec)) {
            return std::unexpected(EncodingError::CannotOpenFile);
        }
    }

    // д������ļ�
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        return std::unexpected(EncodingError::CannotOpenFile);
    }

    outputFile.write(conversionResult->c_str(), conversionResult->length());
    if (!outputFile.good()) {
        return std::unexpected(EncodingError::CannotReadFile);
    }

    outputFile.close();
    return {};
}

Result<std::string> toUTF8(std::string_view input, std::string_view fromEncoding) {
    if (input.empty()) {
        return std::string{};
    }

    std::string sourceEncoding(fromEncoding);
    if (sourceEncoding.empty()) {
        auto detectionResult = detectString(input);
        if (!detectionResult) {
            // ������ʧ�ܣ������Ѿ���UTF-8�򷵻�ԭ�ַ���
            return std::string(input);
        }
        sourceEncoding = *detectionResult;
        if (sourceEncoding == "unknown" || sourceEncoding == "empty") {
            return std::string(input);
        }
    }

    // ����Ѿ���UTF-8��ֱ�ӷ���
    if (sourceEncoding == "UTF-8") {
        return std::string(input);
    }

    return convertString(input, sourceEncoding, "UTF-8");
}

Result<size_t> convertDirectory(const std::filesystem::path& inputDir,
                              const std::filesystem::path& outputDir,
                              std::string_view filePattern,
                              std::string_view fromEncoding,
                              std::string_view toEncoding,
                              bool recursive) {
    std::error_code ec;
    if (!std::filesystem::exists(inputDir, ec) ||
        !std::filesystem::is_directory(inputDir, ec)) {
        return std::unexpected(EncodingError::FileNotFound);
    }

    size_t processedCount = 0;

    auto processFile = [&](const std::filesystem::path& filePath) -> bool {
        const auto relativePath = std::filesystem::relative(filePath, inputDir, ec);
        const auto outputPath = outputDir / relativePath;

        auto result = convertFile(filePath, outputPath, fromEncoding, toEncoding);
        if (result) {
            ++processedCount;
            return true;
        }
        return false;
    };

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir, ec)) {
            if (entry.is_regular_file(ec)) {
                processFile(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(inputDir, ec)) {
            if (entry.is_regular_file(ec)) {
                processFile(entry.path());
            }
        }
    }

    return processedCount;
}

// ��������ʵ��
std::string errorToString(const EncodingError error) {
    switch (error) {
        case EncodingError::FileNotFound:
            return "�ļ�������";
        case EncodingError::NotRegularFile:
            return "·�����ǳ����ļ�";
        case EncodingError::CannotOpenFile:
            return "�޷����ļ�";
        case EncodingError::CannotReadFile:
            return "�޷���ȡ�ļ�";
        case EncodingError::EmptyInput:
            return "����Ϊ��";
        case EncodingError::DetectorCreationFailed:
            return "���������ʧ��";
        case EncodingError::DetectionFailed:
            return "������ʧ��";
        case EncodingError::ConversionFailed:
            return "����ת��ʧ��";
        case EncodingError::InvalidInput:
            return "��Ч����";
        default:
            return "δ֪����";
    }
}

bool isValidEncoding(std::string_view encoding) {
    // �򵥵ı�����֤��������չ
    static const std::vector<std::string_view> validEncodings = {
        "UTF-8", "UTF-16", "UTF-32", "GB18030", "GBK", "GB2312",
        "BIG5", "SHIFT_JIS", "EUC-JP", "EUC-KR", "ISO-8859-1",
        "ASCII", "Windows-1252"
    };

    return std::ranges::any_of(validEncodings,
        [encoding](std::string_view valid) {
            return encoding == valid;
        });
}

std::vector<std::string> getSupportedEncodings() {
    return {
        "UTF-8", "UTF-16", "UTF-32", "GB18030", "GBK", "GB2312",
        "BIG5", "SHIFT_JIS", "EUC-JP", "EUC-KR", "ISO-8859-1",
        "ASCII", "Windows-1252"
    };
}
std::string getConsoleEncoding() {
#ifdef _WIN32
    // Windows ƽ̨
    UINT codepage = GetConsoleOutputCP();
    if (codepage == 0) {
        // ��ȡʧ�ܣ����ؿ��ַ���
        return "";
    }

    // ������ҳת��Ϊ��������
    switch (codepage) {
        case 65001:
            return "UTF-8";
        case 936:
            return "GBK";
        case 950:
            return "BIG5";
        case 932:
            return "SHIFT_JIS";
        case 949:
            return "EUC-KR";
        case 1252:
            return "Windows-1252";
        case 1251:
            return "Windows-1251";
        case 1250:
            return "Windows-1250";
        case 1254:
            return "Windows-1254";
        case 1253:
            return "Windows-1253";
        case 1255:
            return "Windows-1255";
        case 1256:
            return "Windows-1256";
        case 1257:
            return "Windows-1257";
        case 1258:
            return "Windows-1258";
        case 437:
            return "CP437";
        case 850:
            return "CP850";
        case 852:
            return "CP852";
        case 855:
            return "CP855";
        case 857:
            return "CP857";
        case 860:
            return "CP860";
        case 861:
            return "CP861";
        case 862:
            return "CP862";
        case 863:
            return "CP863";
        case 864:
            return "CP864";
        case 865:
            return "CP865";
        case 866:
            return "CP866";
        case 869:
            return "CP869";
        case 874:
            return "Windows-874";
        case 20866:
            return "KOI8-R";
        case 21866:
            return "KOI8-U";
        case 28591:
            return "ISO-8859-1";
        case 28592:
            return "ISO-8859-2";
        case 28593:
            return "ISO-8859-3";
        case 28594:
            return "ISO-8859-4";
        case 28595:
            return "ISO-8859-5";
        case 28596:
            return "ISO-8859-6";
        case 28597:
            return "ISO-8859-7";
        case 28598:
            return "ISO-8859-8";
        case 28599:
            return "ISO-8859-9";
        case 28603:
            return "ISO-8859-13";
        case 28605:
            return "ISO-8859-15";
        case 54936:
            return "GB18030";
        default:
            // ���� CP + ����ҳ�ŵĸ�ʽ
            return std::format("CP{}", codepage);
    }
#else
    // Unix/Linux/macOS ƽ̨
    // ���ȳ��Դ� locale ��ȡ
    const char* locale = setlocale(LC_CTYPE, nullptr);
    if (!locale) {
        return "";
    }

    // ��ȡ�ַ�����Ϣ
    const char* charset = nl_langinfo(CODESET);
    if (!charset || !*charset) {
        return "";
    }

    // ��׼����������
    std::string encoding(charset);

    // ת��Ϊ��д�Ա�Ƚ�
    std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::toupper);

    // ��׼��һЩ�����ı�������
    if (encoding == "UTF8" || encoding == "UTF-8") {
        return "UTF-8";
    } else if (encoding == "GB2312" || encoding == "EUC-CN") {
        return "GB2312";
    } else if (encoding == "GBK" || encoding == "CP936") {
        return "GBK";
    } else if (encoding == "GB18030") {
        return "GB18030";
    } else if (encoding == "BIG5" || encoding == "BIG-5" || encoding == "CP950") {
        return "BIG5";
    } else if (encoding == "EUCJP" || encoding == "EUC-JP") {
        return "EUC-JP";
    } else if (encoding == "SJIS" || encoding == "SHIFT-JIS" || encoding == "CP932") {
        return "SHIFT_JIS";
    } else if (encoding == "EUCKR" || encoding == "EUC-KR") {
        return "EUC-KR";
    } else if (encoding == "ISO88591" || encoding == "ISO-8859-1" || encoding == "LATIN1") {
        return "ISO-8859-1";
    } else if (encoding == "ASCII" || encoding == "US-ASCII" || encoding == "ANSI_X3.4-1968") {
        return "ASCII";
    } else if (encoding == "CP1252" || encoding == "WINDOWS-1252") {
        return "Windows-1252";
    } else if (encoding.find("ISO8859") != std::string::npos ||
               encoding.find("ISO-8859") != std::string::npos) {
        // �������� ISO-8859 ����
        return charset;
    } else {
        // ����ԭʼ�ַ�������
        return charset;
    }
#endif
}

} // namespace encoding
