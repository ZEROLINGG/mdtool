cmake_minimum_required(VERSION 4.0)
project(mdtool2)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdtool2 main.cpp
        main.h
        cli.cpp
        cli.h
        global.h
        tools/tools.cpp
        tools/tools.h
        tools/tool_core/CodeBlock.cpp
        tools/tool_core/CodeBlock.h
        tools/tool_core/shared.cpp
        tools/tool_core/shared.h)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# RE2
find_package(RE2 REQUIRED)
target_link_libraries(mdtool2 PRIVATE re2::re2)

# uchardet
find_library(UCHARDET_LIB NAMES uchardet
        PATHS "D:/AAA/a/msys64/mingw64/lib"
        NO_DEFAULT_PATH
)
message(STATUS "Uchardet library: ${UCHARDET_LIB}")
target_link_libraries(mdtool2 PRIVATE ${UCHARDET_LIB})
include_directories("D:/AAA/a/msys64/mingw64/include")

# iconv
find_library(ICONV_LIB NAMES iconv
        PATHS "D:/AAA/a/msys64/mingw64/lib"
        NO_DEFAULT_PATH
)
message(STATUS "Iconv library: ${ICONV_LIB}")
target_link_libraries(mdtool2 PRIVATE ${ICONV_LIB})
include_directories("D:/AAA/a/msys64/mingw64/include")

