# mdtool - Markdown代码块处理工具

一个用于批量处理Markdown文档中代码块的命令行工具，支持添加、更新、删除代码块语言标记以及删除代码块内容。

## 功能特性

- **添加语言标记**：为没有语言标记的代码块添加指定语言
- **更新语言标记**：强制更新所有代码块的语言标记
- **删除语言标记**：移除所有代码块的语言标记
- **删除代码行**：从代码块开头删除指定行数的内容
- **批量处理**：支持单个文件或整个目录的递归处理
- **编码自动检测**：自动检测并保持文件原始编码（支持UTF-8、GBK、GB2312等）
- **跨平台支持**：支持Windows、Linux和macOS

## 安装

### 依赖项

- CMake 3.31+
- C++23编译器
- uchardet（字符编码检测库）
- iconv（字符编码转换库，Windows和macOS需要）
- re2（正则库）

### 编译

```bash
git clone https://github.com/ZEROLINGG/mdtool.git
cd mdtool
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 基本语法

```bash
mdtool [OPTIONS]
```

### 选项说明

| 选项 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-v, --version` | 显示版本信息 |
| `-p, --path <路径>` | **必需**。指定单个md文档路径或包含md文档的文件夹路径 |
| `-l, --language <语言>` | 指定代码语言（如：cpp、python、java等） |
| `-n, --line <行数>` | 指定要删除的行数 |
| `--addl, --add-language` | 为没有语言标记的代码块添加语言 |
| `--updl, --update-language` | 强制更新所有代码块的语言标记 |
| `--rmvl, --remove-language` | 删除所有代码块的语言标记 |
| `--delcl, --delete-code-line` | 从代码块开头删除指定行数 |

**注意**：`--addl`、`--updl`、`--rmvl`、`--delcl` 四个操作选项互斥，一次只能使用其中一个。

### 使用示例

#### 1. 为单个文件添加语言标记

```bash
# 为没有语言标记的代码块添加cpp标记
mdtool --addl -l cpp -p /path/to/document.md
```

#### 2. 更新目录中所有文件的语言标记

```bash
# 将目录中所有md文件的代码块语言更新为python
mdtool --updl -l python -p /path/to/markdown/folder
```

#### 3. 删除所有语言标记

```bash
# 删除文件中所有代码块的语言标记
mdtool --rmvl -p /path/to/document.md
```

#### 4. 删除代码块的前几行

```bash
# 删除所有代码块的前3行
mdtool --delcl -n 3 -p /path/to/document.md
```

## 工作原理

### 代码块识别

工具使用正则表达式识别Markdown中的代码块：
- 识别以 ` ``` ` 开始和结束的代码块
- 保留代码块的缩进和格式
- 正确处理嵌套和特殊格式

### 编码处理

1. 自动检测文件原始编码
2. 转换为UTF-8进行处理
3. 处理完成后转换回原始编码
4. 支持的编码包括：UTF-8、GBK、GB2312、BIG5、SHIFT_JIS等

### 文件处理流程

1. 验证输入路径
2. 检测路径类型（文件/目录）
3. 对每个Markdown文件：
    - 读取并检测编码
    - 执行指定操作
    - 保持原始编码写回

## 项目结构

```
mdtool/
├── CMakeLists.txt      # CMake配置文件
├── main.cpp            # 程序入口
├── cli.cpp/h           # 命令行参数处理
├── tools/              # 核心功能实现
│   ├── tools.cpp/h     # 工具函数接口
│   └── tool_core.cpp/h # 核心处理逻辑
└── other/              # 辅助功能
    ├── coding.cpp/h    # 编码检测与转换
    ├── path.cpp/h      # 路径处理
    └── log.h           # 日志输出
```

## 注意事项

1. **备份重要文件**：工具会直接修改原文件，建议处理前备份
2. **大目录处理**：处理大目录时有1.5秒超时限制，防止误操作
3. **编码兼容性**：虽然支持多种编码，但建议统一使用GBK或UTF-8
4. **代码块格式**：确保代码块使用标准的三个反引号格式



## 贡献

欢迎提交Issue和Pull Request！

## 作者

ZEROLINGG
