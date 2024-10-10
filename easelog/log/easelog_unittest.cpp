/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog_unittest.cpp
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-09 23:34
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  实现EaseLog的单元测试.
 *
 */

#include "log/easelog.h"
#include "log/easelog_private.h"

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>

#include <sstream>
#include <string>
#include <optional>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logging {

using ::testing::_;
using ::testing::Return;

class MockLogSource {
public:
    MOCK_METHOD0(Log, const char *());
};

TEST(LoggingTestBase, BasicLoggingLog)
{
    MockLogSource mock_log_source;

    // 4 base logs: LOG, LOG_IF
    int32_t expected_logs = 2;

    /* 指定测试对象行为, 调用次数和返回值 */
    EXPECT_CALL(mock_log_source, Log())
        .Times(expected_logs)
        .WillRepeatedly(Return("log message test"));

    /* 设置日志级别并且进行验证 */
    SetMinLogLevel(LOGGING_INFO);
    EXPECT_TRUE(LOG_IS_ON(INFO));

    LOG(INFO) << mock_log_source.Log();
    LOG_IF(INFO, true) << mock_log_source.Log();
    /* 条件为假, 不会产生调用次数 */
    LOG_IF(INFO, false) << mock_log_source.Log();
}

#define REPEAT_TIMES 1000

// 简易性能测试, 输入1000条日志, 记录耗时
TEST(LoggingTestBase, BasicPerformance)
{
    uint64_t start_time = TickCountUs();

    for (int i = 0; i < REPEAT_TIMES; i++) {
        LOG(INFO) << "log message test";
    }

    uint64_t end_time = TickCountUs();

    std::cout << "Performance test: " << REPEAT_TIMES << "times * "
              << (end_time - start_time) / REPEAT_TIMES << " us" << std::endl;
}

#undef REPEAT_TIMES
#define REPEAT_TIMES 20

static bool g_log_enable_random_sleep = false;

// 用于构造并发时序, 随机等待 10-50ms
void RandomSleep()
{
    if (g_log_enable_random_sleep) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 40));
    }
}

// 测试多线程写日志
TEST(LoggingTestBase, MultiThreadLogging)
{
    g_log_enable_random_sleep = true;

    // 线程名字 thread1, thread2, thread3
    std::thread t1([]() {
        pthread_setname_np(pthread_self(), "thread1");
        for (int i = 0; i < REPEAT_TIMES; i++) {
            LOG(INFO) << "log message test: " << i;
        }
    });

    std::thread t2([]() {
        pthread_setname_np(pthread_self(), "thread2");
        for (int i = 0; i < REPEAT_TIMES; i++) {
            LOG(INFO) << "log message test: " << i;
        }
    });

    std::thread t3([]() {
        pthread_setname_np(pthread_self(), "thread3");
        for (int i = 0; i < REPEAT_TIMES; i++) {
            LOG(INFO) << "log message test: " << i;
        }
    });

    t1.join();
    t2.join();
    t3.join();
}

}    // namespace logging
