//
// Created by zerox on 25-8-10.
//

#include "path.h"

#include "log.h"

std::filesystem::path to_fs_path(const std::string& path_str) {
#ifdef _WIN32
    const int wideLen = MultiByteToWideChar(CP_ACP, 0, path_str.c_str(), -1, nullptr, 0);
    if (wideLen == 0) {
        LOG_ERROR("·��ת��ʧ��: " + path_str);
        return { std::wstring{} };
    }

    std::wstring wideStr(wideLen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, path_str.c_str(), -1, &wideStr[0], wideLen);
    // ȥ��ĩβ�����'\0'
    wideStr.resize(wideLen - 1);

    return { wideStr };
#else
    return { path_str };
#endif
}

std::string out_fs_path(const std::filesystem::path& path) {
#ifdef _WIN32
    const std::wstring& wideStr = path.native();
    // �Ȼ�ȡת������ַ������ȣ�����ĩβ'\0'��
    const int narrowLen = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (narrowLen == 0) {
        LOG_ERROR("·��ת��ʧ��: ");
        std::wcerr << wideStr << std::endl;

        return {};
    }

    std::string narrowStr(narrowLen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &narrowStr[0], narrowLen, nullptr, nullptr);
    // ȥ��ĩβ�����'\0'
    narrowStr.resize(narrowLen - 1);

    return narrowStr;
#else
    return path.string();
#endif
}
