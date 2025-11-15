//
// Created by zerox on 2025/11/7.
//

#include "cli.h"


cxxopts::Options cmd_opts("mdtool", "Markdown代码块处理工具");


FinalFuncReturn help(const InputOptions& _) {
    std::cout << cmd_opts.help() << std::endl;
    FinalFuncReturn r;
    r.success = true;
    return r;
}
FinalFuncReturn version(const InputOptions& _) {
    const std::string v = "2.0.0";
    std::cout << "mdtool v" << v << std::endl;
    FinalFuncReturn r;
    r.success = true;
    return r;
}
FinalFuncReturn none(const InputOptions& _) {
    FinalFuncReturn r;
    r.logs = _.logs;
    r.success = false;
    return r;
}


CommandReturn commandParsing(const int argc, char* argv[]) {
    InputOptions options;
    oldOptions oldOptions{};
    std::vector<std::string> eOptions;

    CommandReturn cmd_ret;
    cmd_ret.funcPtr = nullptr;
    cmd_ret.success = false;

    cmd_opts.add_options()
    ("h,help", "显示帮助信息")
    ("v,version", "显示版本信息");

    cmd_opts.add_options("旧版操作选项")
    ("addl,add-language", "为代码块添加语言(如果原本没有)",
        cxxopts::value<bool>(oldOptions.addl)->implicit_value("true")->default_value("false"))
    ("updl,update-language", "强制更新代码块的语言",
        cxxopts::value<bool>(oldOptions.updl)->implicit_value("true")->default_value("false"))
    ("rmvl,remove-language", "删除所有代码块的语言",
        cxxopts::value<bool>(oldOptions.rmvl)->implicit_value("true")->default_value("false"))
    ("delc,delete-code", "删除代码块首位指定行",
        cxxopts::value<bool>(oldOptions.delcl)->implicit_value("true")->default_value("false"));

    cmd_opts.add_options("统一操作选项")
    ("e,execute",R"(
统一的操作输入选项
    使用方式：
      操作对象 操作内容 附加参数 （cb.lf add）
      快捷操作 （addl）
    操作详情：
      cb （代码块）包含li，ct           {add，upd，rmv，format(去除多余换行，空白字符);add,rmv,format}
      mh （多级标题）                  {add1,sub1,add2,sub2,add3,sub3,}
      il （内部链接）                  {}
      el （外部链接）

)",cxxopts::value<std::vector<std::string>>(eOptions));

    cmd_opts.add_options("操作参数")
    ("p,path", "单个md文档路径,或包含md文档的文件夹路径",
        cxxopts::value<std::string>(options.path_str))
    ("i,input","操作所需要的输入内容",
        cxxopts::value<std::string>(options.input))
    ("if,input-file","从文件读取操作所需要的输入内容",
        cxxopts::value<std::string>(options.input_file))
    ("n,number", "指定在对象中操作的位置或数量",
        cxxopts::value<int>(options.number)->default_value("0"))
    ("s,start","指定md文档中操作的起始位置",
        cxxopts::value<int>(options.start))
    ("l,log","设置日志输出级别，默认3",
        cxxopts::value<int>(options.log))
    ("b,backup","指定是否使用备份",
        cxxopts::value<std::optional<bool>>(options.bakup)->implicit_value("true")->default_value("false"))  ;




    try {
        const auto result = cmd_opts.parse(argc, argv);
        cmd_ret.options = options;
        if (result.count("help")) {
            cmd_ret.funcPtr = help;
            cmd_ret.success = true;
            return cmd_ret;
        }
        if (result.count("version")) {
            cmd_ret.funcPtr = version;
            cmd_ret.success = true;
            return cmd_ret;
        }


        // 其他解析

    } catch (const std::exception& e) {
        options.logs = {logs::alog(LOG_TYPE::Error, e.what())};
        cmd_ret.funcPtr = none;
        cmd_ret.success = false;
        return cmd_ret;
    }


    return cmd_ret;
}
