#include <CLI/CLI.hpp>
#include <string>
#include "cli.h"
#include <filesystem>


#include "other/log.h"
#include "other/path.h"
#include "other/coding.h"
#include "tools/tools.h"

CliReturn cli_work(const int argc, char* argv[]) {


    CLI::App app{"mdtool - Markdown����鴦����"};
    Options options;
    std::string path_str;
    std::string language;

    // �������
    app.add_option("-p,--path", path_str, "����md�ĵ�·���������md�ĵ����ļ���·��")
        ->required();
    // ��ѡ����
    app.add_option("-l,--language", language, "ָ����������");
    app.add_option("-n,--line", options.line, "ָ������");

    // ������־�����⣩
    auto* addl_flag = app.add_flag("--addl,--add-language", options.addl,
        "Ϊ``` ```������������(���ԭ��û��)");
    auto* updl_flag = app.add_flag("--updl,--update-language", options.updl,
        "ǿ�Ƹ���``` ```����������");
    auto* rmvl_flag = app.add_flag("--rmvl,--remove-language", options.rmvl,
        "ɾ������``` ```����������");
    auto* delcl_flag = app.add_flag("--delcl,--delete-code-line", options.delcl,
        "ɾ��``` ```�����ӵ�һ��```��ʼ����line��");

    // ���ò�����־����
    addl_flag->excludes(updl_flag)->excludes(rmvl_flag)->excludes(delcl_flag);
    updl_flag->excludes(rmvl_flag)->excludes(delcl_flag);
    rmvl_flag->excludes(delcl_flag);

    CliReturn ret;
    ret.funcPtr = nullptr;
    ret.success = false;

    try {
        app.parse(argc, argv);

        // ���û������·���ַ���ת�� std::filesystem::path
        options.path = to_fs_path(path_str);
        if (!std::filesystem::exists(options.path)) {
            LOG_ERROR("·������: " + path_str);
            ret.success = false;
            return ret;
        }

        // ת�����벢����Ƿ�ɹ�
        if (auto utf8_result = encoding::toUTF8(language, encoding::getConsoleEncoding())) {
            options.language = *utf8_result;  // ��ȡ�ɹ����
        } else {
            LOG_ERROR("���Ա���ת��ʧ��");
            ret.success = false;
            return ret;
        }

        ret.success = true;
        ret.options = options;
    } catch ([[maybe_unused]] const CLI::ParseError &e) {
        std::cerr << app.help() << std::endl;
        ret.success = false;
        return ret;
    }

    if (options.addl) {
        ret.funcPtr = tools::addl;
    } else if (options.updl) {
        ret.funcPtr = tools::updl;
    } else if (options.rmvl) {
        ret.funcPtr = tools::rmvl;
    } else if (options.delcl) {
        ret.funcPtr = tools::delcl;
    }

    return ret;
}
