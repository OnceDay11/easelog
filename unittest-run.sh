###
# SPDX-License-Identifier: BSD-3-Clause
#
# @Filename: test-run.sh
# @Author: Once Day <once_day@qq.com>.
# @Date: 2023-11-30 10:46.
# @Info: Encoder=utf-8, Tabsize=4, EOL=\n.
#
# @Description:
#    构建测试程序Host system运行环境, 在内存沙盒里运行.
#
###

# 开启shell调试
# set -x

# 构建Host运行环境
# export LD_LIBRARY_PATH=./output/lib/:$LD_LIBRARY_PATH

# 设置coredump文件的生成路径
# ulimit -c

# 允许生成coredump文件
# echo "|gzip -c > ${PWD}/crash/%e-%p-%t-%s-${TARGET_VERSION}.coredump.gz" | \
#    sudo tee /proc/sys/kernel/core_pattern
# STR="${OUTPUT_DIR}/crash/%e-%p-%t-%s-${TARGET_VERSION}.coredump"
# sudo bash -c "echo '${STR}' > /proc/sys/kernel/core_pattern"

# 创建crash文件夹
# if [ ! -d ${OUTPUT_DIR}/crash ]; then
#     mkdir -p ${OUTPUT_DIR}/crash
# fi

# 开始执行所有文件
echo "***** Start run all test programs *****"

# 内存检测工具 Valgrind:
# --tool=memcheck: 使用valgrind的memcheck内存检测工具
# --leak-check=full: 进行完整的内存泄漏检测
# --show-leak-kinds=all: 显示所有类型的内存泄漏
# --track-origins=yes: 跟踪内存泄漏的来源,即泄漏发生的具体位置
# 所以总结起来,这个valgrind命令的作用是:
#   使用memcheck工具,进行完整和详细的内存泄漏检测,显示所有的内存泄漏信息,并跟踪泄漏发生的准确位置。
#   通过使用这些参数,可以非常方便和详细地调试程序中的内存泄漏问题。
#   memcheck工具会打印每个泄漏发生的堆栈信息,以及泄漏的大小等信息。
MEMCHECK="valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes -s"

# 添加额外规则, 追踪子进程, 必要时候生成抑制规则
MEMCHECK="${MEMCHECK} --trace-children=yes --suppressions=valgrind-ignore.txt"
if [ -n $ENABLE_SUPPRESSION ]; then
    MEMCHECK="${MEMCHECK} --gen-suppressions=yes"
fi

# 排除在外的单元测试用例, 主要是CheckDeathTest
EXECUTE_ARGS="--gtest_filter=-CheckDeathTest*"

# 如果$@为空, 则执行默认的测试程序, 否则执行传入的所有参数
if [ $# -eq 0 ]; then
    # 执行默认的测试程序
    echo "Run default test program: easelog-tests"
    $MEMCHECK ./output/bin/easelog-tests $EXECUTE_ARGS
    echo "\n"
else
    # 计数变量
    count=0
    # 遍历传入的所有参数
    for program_path in $@; do
        # 计数
        count=$((count + 1))
        program_name=$(basename $program_path)
        echo "[${count}]run file: ${program_name}"
        $MEMCHECK $program_path $EXECUTE_ARGS
        echo "\n"
    done
fi

# 执行完毕
echo "***** Run all test file *****"
