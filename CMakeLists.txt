###
 # SPDX-License-Identifier: BSD-3-Clause
 #
 # Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 #
 # @FilePath: /CMakeLists.txt
 # @Author: Once Day <once_day@qq.com>.
 # @Date: 2024-08-16 14:42
 # @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 #
###

# cmake版本号要求
cmake_minimum_required(VERSION 3.0)

# 项目名称/版本号
project(easelog VERSION 1.0.0)

# 使能编译语言 C C++ ASM
enable_language(C CXX ASM)

# 设置编译器, 使用-DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++指定
# set(CMAKE_C_COMPILER "/usr/bin/gcc")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

# 记录顶层目录, 子目录可以使用
set(TOPDIR ${CMAKE_CURRENT_SOURCE_DIR})

# 测试使能
enable_testing()
# 设置Gtest的目录
# set(GTEST_ROOT /usr/local/gtest)

# 设置编译类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# only build examples if this is the main project
# if(CMAKE_PROJECT_NAME STREQUAL "muduo")
#   option(MUDUO_BUILD_EXAMPLES "Build Muduo examples" ON)
# endif()

# 设置输出二进制类型为位置无关代码
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(C_FLAGS
    -g
    # -DVALGRIND
    -D_GNU_SOURCE=1         # GNU扩展
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -Wall                   # 打开所有警告
    -Wextra                 # 打开额外警告
    -Werror                 # 严格的错误处理策略
    -Wshadow                # 检查变量声明遮蔽
    -Wundef                 # 未定义的宏
    -Wcast-qual             # 类型转换时丢失限定符
    -Wcast-align            # 类型转换时对齐
    -Wstrict-prototypes     # 函数原型声明
    -Wmissing-prototypes    # 缺少函数原型声明
    -Wmissing-declarations  # 缺少声明
    -Wnested-externs        # 嵌套的extern声明
    -Wunreachable-code      # 不可达的代码
    -Wuninitialized         # 未初始化的变量
    -Winline                # 内联函数
    -Wfloat-equal           # 浮点数比较
    -Wdouble-promotion      # 浮点数提升
    -Wswitch                # 检查switch语句中的枚举值, switch case集合必须和枚举定义集合一致
    -Wswitch-default        # 检查switch语句中的default, 没有default将报错
    -Wbad-function-cast     # 检查函数指针强制转换
    -Waggregate-return      # 不允许返回结构体或联合体
    -Wpacked                # 检查结构体或联合体的字节对齐, 如果对齐不合理, 将报错
    -Wpadded                # 检查结构体或联合体的字节对齐, 如果额外填充了字节, 将报错
    -Wvariadic-macros       # 检查宏定义
    -Wvla                   # 变长数组
    -Wconversion            # 检查隐式类型转换
    -Wsign-conversion       # 检查隐式类型转换
    -Wpointer-arith         # 指针运算
    -Wwrite-strings         # 字符串常量
    -Woverlength-strings    # 字符串常量
    -Wpedantic              # 严格遵循ISO C/C++标准，检查编译器扩展语法
    -Wformat-overflow=2     # 格式化字符串溢出
    -Wformat=2              # 格式化字符串
    -Wformat-truncation=2   # 格式化字符串截断
    -Walloc-zero            # 分配内存时检查是否为0
    -Wnull-dereference      # 空指针解引用
    -Wno-unused-parameter   # 未使用的参数
    -Wno-unused-variable    # 未使用的变量
    -Wno-unused-function    # 未使用的函数
    -Wduplicated-cond       # 重复的条件
    -Wduplicated-branches   # 重复的分支
    -Wlogical-op            # 逻辑操作符
    -Wjump-misses-init      # 跳转到未初始化的变量
    -Wmisleading-indentation   # 误导性缩进
    -Wsizeof-pointer-memaccess # 访问指针大小
    # -MMD
    -march=native           # 优化
    -fstack-protector-all   # 栈保护
    -std=c11                # C11标准
    # -rdynamic             # 生成位置无关代码
)

# 设置编译选项, 开启调试信息, 定义宏, 开启警告, 使用C++11标准, 生成位置无关代码
set(CXX_FLAGS
    -g
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -Wall                   # 打开所有警告
    -Wextra                 # 打开额外警告
    -Werror                 # 严格的错误处理策略
    -Wshadow                # 检查变量声明遮蔽
    -Wundef                 # 未定义的宏
    -Wcast-qual             # 类型转换时丢失限定符
    -Wcast-align            # 类型转换时对齐
    -Wunreachable-code      # 不可达的代码
    -Wuninitialized         # 未初始化的变量
    -Winline                # 内联函数
    -Wfloat-equal           # 浮点数比较
    -Wdouble-promotion      # 浮点数提升
    -Wswitch                # 检查switch语句中的枚举值, switch case集合必须和枚举定义集合一致
    -Wswitch-default        # 检查switch语句中的default, 没有default将报错
    #-Waggregate-return      # 不允许返回结构体或联合体
    -Wpacked                # 检查结构体或联合体的字节对齐, 如果对齐不合理, 将报错
    -Wpadded                # 检查结构体或联合体的字节对齐, 如果额外填充了字节, 将报错
    -Wvariadic-macros       # 检查宏定义
    -Wvla                   # 变长数组
    -Wconversion            # 检查隐式类型转换
    -Wsign-conversion       # 检查隐式类型转换
    -Wpointer-arith         # 指针运算
    -Wwrite-strings         # 字符串常量
    -Woverlength-strings    # 字符串常量
    -Wpedantic              # 严格遵循ISO C/C++标准，检查编译器扩展语法
    -Wformat-overflow=2     # 格式化字符串溢出
    -Wformat=2              # 格式化字符串
    -Wformat-truncation=2   # 格式化字符串截断
    -Walloc-zero            # 分配内存时检查是否为0
    -Wnull-dereference      # 空指针解引用
    -Wno-unused-parameter   # 未使用的参数
    -Wno-unused-variable    # 未使用的变量
    -Wno-unused-function    # 未使用的函数
    -Wold-style-cast        # 旧式类型转换
    -Woverloaded-virtual    # 虚函数重载
    -Wduplicated-cond       # 重复的条件
    -Wduplicated-branches   # 重复的分支
    -Wlogical-op            # 逻辑操作符
    -Wmisleading-indentation   # 误导性缩进
    -Wsizeof-pointer-memaccess # 访问指针大小
    # -MMD
    -march=native           # 优化
    -fstack-protector-all   # 栈保护
    -std=c++11              # C++11标准
    # -rdynamic             # 生成位置无关代码
)

# 设置编译选项
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()

# 启用覆盖率测试
option(COVERAGE "Enable coverage reporting" OFF)
if(COVERAGE)
  list(APPEND CXX_FLAGS "--coverage")
  list(APPEND C_FLAGS "--coverage")
endif()

# 设置编译选项
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

# 设置debug版本编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
# 支持大栈帧, 大量使用栈内存
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 --param large-stack-frame=2048")
# 设置release版本编译选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g3 -DNDEBUG")
# 设置二进制文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/output/bin)
# 设置库文件输出目录
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/output/lib)

# 添加头文件搜索路径
# include_directories(${PROJECT_SOURCE_DIR})

# 选择编译类型对应的编译标识
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "C_FLAGS = " ${CMAKE_C_FLAGS} " " ${CMAKE_C_FLAGS_${BUILD_TYPE}})
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# 添加子目录
add_subdirectory(easelog)

# Ends here.
