cmake_minimum_required(VERSION 3.31)
project(mdtool)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -ffunction-sections -fdata-sections -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -s")

add_executable(mdtool main.cpp
        cli.cpp
        tools/tools.h
        cli.h
        tools/tools.cpp
        tools/tool_core.cpp
        tools/tool_core.h
        other/path.cpp
        other/path.h
        other/log.h
        other/coding.cpp
        other/coding.h
)
target_link_libraries(mdtool PRIVATE
        uchardet
        re2
)
if (WIN32 OR APPLE)
    target_link_libraries(mdtool PRIVATE iconv)
endif()