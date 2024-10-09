#!/bin/sh

# 开启shell调试
#set -x

# 当前目录路径
SOURCE_DIR=$(pwd)

# 指定默认编译目录
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}

# 指定默认编译类型
BUILD_TYPE=debug
BUILD_TYPE=${BUILD_TYPE:-release}

# 指定默认安装目录
INSTALL_DIR=${INSTALL_DIR:-${SOURCE_DIR}/${BUILD_TYPE}-install-cpp11}

# 指定默认编译器
CXX=${CXX:-g++}

# 创建编译命令描述文件的符号链接
ln -sf $BUILD_DIR/$BUILD_TYPE-cpp11/compile_commands.json

# 创建编译链接描述文件的符号链接
ln -sf $BUILD_DIR/$BUILD_TYPE-cpp11/easelog/CMakeFiles/easelog-tests.dir/link.txt

# 判断是否开启gcov编译
if [ "$1" = "--gcov" ]; then
    GCOV=ON
    # 清除之前的gcov文件
    find $BUILD_DIR/$BUILD_TYPE-cpp11 -name "*.gcda" -delete
    find $BUILD_DIR/$BUILD_TYPE-cpp11 -name "*.gcno" -delete
    rm output/coverage/* -rf
    make clean -C $BUILD_DIR/$BUILD_TYPE-cpp11
else
    GCOV=OFF
fi

# 创建编译目录
mkdir -p $BUILD_DIR/$BUILD_TYPE-cpp11 &&
    cd $BUILD_DIR/$BUILD_TYPE-cpp11 &&
    cmake \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCOVERAGE=$GCOV \
        $SOURCE_DIR &&
    make V=1 -j$(nproc)

# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# 处理gcov文件
# if [ "$GCOV" = "ON" ]; then
#     # 拷贝所有的gcda和gcno到output/coverage目录
#     find $BUILD_DIR/$BUILD_TYPE-cpp11 -name "*.gcda" -exec cp -v {} $SOURCE_DIR/output/coverage \;
#     find $BUILD_DIR/$BUILD_TYPE-cpp11 -name "*.gcno" -exec cp -v {} $SOURCE_DIR/output/coverage \;
# fi

# cd $SOURCE_DIR && doxygen
