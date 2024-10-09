#!/bin/sh

# 开启shell调试
# set -x

# 指定GCOV_PREFIX运行程序
# GCOV_PREFIX="GCOV_PREFIX_STRIP=9 GCOV_PREFIX=output/coverage"

# 执行所有的单元测试
./output/bin/anmk-base-tests

# 生成单元测试信息
OUTPUT_DIR=output/coverage
OUTPUT_FILE=$OUTPUT_DIR/coverage.info
# 生成gcov文件
lcov --capture --directory . --output-file $OUTPUT_FILE
# 移除不需要的文件
lcov --remove $OUTPUT_FILE '/usr/*' --output-file $OUTPUT_FILE
# 生成html文件
genhtml $OUTPUT_FILE --output-directory $OUTPUT_DIR

# 移动文件到nginx html目录
# sudo mkdir -p /var/www/html/coverage/
# sudo cp -r output/coverage/* /var/www/html/coverage/

# 打包为coverage.tar.gz文件
# cd output
# tar -zcvf coverage.tar.gz coverage/
