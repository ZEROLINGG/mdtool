#include <CLI/CLI.hpp>
#include <string>
#include "cli.h"
#include <filesystem>


#include "other/log.h"
#include "other/path.h"
#include "other/coding.h"
#include "tools/tools.h"

CliReturn cli_work(const int argc, char* argv[]) {


    CLI::App app{"mdtool - Markdown代码块处理工具"};
    Options options;
    std::string path_str;
    std::string language;

    // 必需参数
    app.add_option("-p,--path", path_str, "单个md文档路径，或包含md文档的文件夹路径")
        ->required();
    // 可选参数
    app.add_option("-l,--language", language, "指定代码语言");
    app.add_option("-n,--line", options.line, "指定行数");

    // 操作标志（互斥）
    auto* addl_flag = app.add_flag("--addl,--add-language", options.addl,
        "为``` ```代码块添加语言(如果原本没有)");
    auto* updl_flag = app.add_flag("--updl,--update-language", options.updl,
        "强制更新``` ```代码块的语言");
    auto* rmvl_flag = app.add_flag("--rmvl,--remove-language", options.rmvl,
        "删除所有``` ```代码块的语言");
    auto* delcl_flag = app.add_flag("--delcl,--delete-code-line", options.delcl,
        "删除``` ```代码块从第一个```开始到第line行");

    // 设置操作标志互斥
    addl_flag->excludes(updl_flag)->excludes(rmvl_flag)->excludes(delcl_flag);
    updl_flag->excludes(rmvl_flag)->excludes(delcl_flag);
    rmvl_flag->excludes(delcl_flag);

    CliReturn ret;
    ret.funcPtr = nullptr;
    ret.success = false;

    try {
        app.parse(argc, argv);

        // 将用户输入的路径字符串转成 std::filesystem::path
        options.path = to_fs_path(path_str);
        if (!std::filesystem::exists(options.path)) {
            LOG_ERROR("路径错误: " + path_str);
            ret.success = false;
            return ret;
        }

        // 转换编码并检查是否成功
        if (auto utf8_result = encoding::toUTF8(language, encoding::getConsoleEncoding())) {
            options.language = *utf8_result;  // 获取成功结果
        } else {
            LOG_ERROR("语言编码转换失败");
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
