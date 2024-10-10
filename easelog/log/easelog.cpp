/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog.cpp
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-09 22:03
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  实现EaseLog日志处理逻辑.
 *
 */

#include "easelog.h"
#include "easelog_private.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <paths.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// debug: 临时导入iostream用于测试
#include <iostream>

#include <iomanip>
#include <ostream>
#include <string>
#include <utility>
#include <mutex>

#include "log/easelog_llqueue.h"

namespace logging {

// warn: 注意, 这里不使用匿名命名空间, 继续使用static, 匿名空间调试时不好指定对应的函数.
// 在C++中, 匿名命名空间和static具有类似的作用, 即限定变量作用域, 但这会导致符号名字比较随机.
// 为了方便调试, 这里使用static限定符.

// 这里希望使用指定初始化器(designated initializers)的语法, 但是C++20标准才支持.

// note: 结构体可以当成所有成员都是公有成员的全局单例类来使用, 注意这是一个基础聚合类型
// google styles禁止全局类初始化和销毁, 也不推荐使用Singleton模式, 主要是存在时序问题.
// 所以在这里, 使用结构体来模拟全局类, 但是不提供初始化和销毁函数, 程序运行时就存在, 且不会被销毁.
static struct LoggingSettings g_logging_settings = {
    /* .log_file_path       = */ nullptr,
    /* .log_file            = */ nullptr,
    /* .log_min_level       = */ LOGGING_INFO,
    /* .log_always_print    = */ LOGGING_ERROR,
    /* .log_dest            = */ LOG_DEFAULT,
    /* .log_process_id      = */ true,
    /* .log_thread_id       = */ true,
    /* .log_timestamp       = */ true,
    /* .log_tickcount       = */ false,
    /* .log_prefix          = */ nullptr,
    /* .log_severity_names  = */ {"debug", "info", "warning", "error", "fatal"},
};

// 定义日志配置操作宏, 便于后续更改
#define log_settings g_logging_settings

// 获取默认日志配置结构体的引用
const struct LoggingSettings &GetLoggingSettings()
{
    return g_logging_settings;
}

// warn: This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
std::ostream *g_swallow_stream = nullptr;

// 定义无锁队列
#define ASYNC_QUEUE_SIZE 4096
static struct llqueue       g_log_free_queue;
static struct llqueue       g_log_wait_queue;
static struct llqueue_entry g_log_queue_entries[ASYNC_QUEUE_SIZE];

// 初始化日志队列
void InitLoggingQueue()
{
    llqueue_init(&g_log_free_queue, g_log_queue_entries, ASYNC_QUEUE_SIZE);
    llqueue_init(&g_log_wait_queue, g_log_queue_entries, ASYNC_QUEUE_SIZE);
    for (uint32_t i = 0; i < ASYNC_QUEUE_SIZE; i++) {
        llqueue_enqueue(&g_log_free_queue, i);
    }
}

// 全局互斥锁
static std::mutex g_log_mutex;

// export: 日志初始化函数, 用于设置日志配置
bool BaseInitLoggingImpl(const LoggingSettings &settings)
{
    // MaybeInitializeVlogInfo();
    log_settings = settings;

    // Ignore file options unless logging to file is set.
    if ((log_settings.log_dest & LOG_TO_FILE) == 0) {
        return true;
    }

    if (!log_settings.log_file_path) {
        log_settings.log_file_path = new std::string("debug.log");
    }
    *(log_settings.log_file_path) = *settings.log_file_path;

    // return InitializeLogFileHandle();
    return true;
}

// export: 设置日志等级
void SetMinLogLevel(int32_t level)
{
    log_settings.log_min_level = std::min(LOGGING_FATAL, level);
}

// export: 获取日志等级
int32_t GetMinLogLevel()
{
    return log_settings.log_min_level;
}

// export: 设置日志输出等级
bool ShouldCreateLogMessage(int32_t severity)
{
    if (severity < log_settings.log_min_level) {
        return false;
    }

    // Return true here unless we know ~LogMessage won't do anything.
    return log_settings.log_dest != LOG_NONE || severity >= log_settings.log_always_print;
}

// Returns true when LOG_TO_STDERR flag is set, or |severity| is high.
// If |severity| is high then true will be returned when no log destinations are
// set, or only LOG_TO_FILE is set, since that is useful for local development
// and debugging.
static inline bool ShouldLogToStderr(int32_t severity)
{
    if (log_settings.log_dest & LOG_TO_STDERR) {
        return true;
    }

    if (severity >= log_settings.log_always_print) {
        return (log_settings.log_dest & ~LOG_TO_FILE) == LOG_NONE;
    }
    return false;
}

// export: 设置日志信息配置
void SetLogItems(bool enable_process_id, bool enable_thread_id, bool enable_timestamp,
    bool enable_tickcount)
{
    log_settings.log_process_id = enable_process_id;
    log_settings.log_thread_id  = enable_thread_id;
    log_settings.log_timestamp  = enable_timestamp;
    log_settings.log_tickcount  = enable_tickcount;
}

// export: 设置日志前缀
void SetLogPrefix(const char *prefix)
{
    // BUG:需要检查prefix是否为正常的26个字母和数字组成
    log_settings.log_prefix = prefix;
}

static void WriteToFd(int fd, const char *data, size_t length)
{
    size_t bytes_written = 0;
    long   rv;
    while (bytes_written < length) {
        HANDLE_EINTR(rv, write(fd, data + bytes_written, length - bytes_written));
        // Give up, nothing we can do now.
        if (rv < 0) {
            break;
        }
        bytes_written += static_cast< size_t >(rv);
    }
}

// 构造函数: 从文件名和行号构造日志消息, 需要指定日志等级
LogMessage::LogMessage(const char *file, const char *func, int line, LogSeverity severity)
    : file_(file), func_(func), line_(line), severity_(severity)
{
    Init(file, func, line);
}

// 构造函数: 从文件名和行号构造日志消息, 默认日志等级Fatal, 需要指定判断条件, 用于CHECK宏.
LogMessage::LogMessage(const char *file, const char *func, int line, const char *condition)
    : file_(file), func_(func), line_(line), severity_(LOGGING_FATAL)
{
    Init(file, func, line);
    stream_ << "Check failed: " << condition << ". ";
}

// 析构函数: 用于刷新日志消息, 释放资源
LogMessage::~LogMessage()
{
    Flush();
}

void LogMessage::Flush()
{
    // Don't let actions from this method affect the system error after returning.
    ScopedClearLastError scoped_clear_last_error;

    size_t stack_start = stream_.str().length();

    // Include a stack trace on a fatal, unless a debugger is attached.
    if (severity_ == LOGGING_FATAL) {
        /* bug: Fatal时输出关键的debug信息 */
    }

    // note: 默认尾部填充换行符'\n'
    stream_ << std::endl;
    std::string str_newline(stream_.str());

    // 定义自动清理资源的对象, 用于释放资源
    ScopedCleanUp cleanup([&] {
        // If the log message is fatal, handle it.
        if (severity_ == LOGGING_FATAL) {
            HandleFatal(stack_start, str_newline);
        }
    });

    // // 移动到无锁队列中
    // uint32_t entry_idx = llqueue_dequeue(&g_log_free_queue);
    // if (entry_idx == LLQUEUE_NULL_IDX) {
    //     // 如果队列满了, 则直接输出到标准错误
    //     WriteToFd(STDERR_FILENO, str_newline.data(), str_newline.size());
    //     return;
    // }

    // // 将日志信息写入到队列中
    // g_log_queue_entries[entry_idx].data = new std::string(str_newline);
    // llqueue_enqueue(&g_log_wait_queue, entry_idx);

    // // 上锁开始准备写日志
    // {
    //     std::lock_guard< std::mutex > lock(g_log_mutex);
    //     // 从队列中取出日志信息
    //     entry_idx = llqueue_dequeue_all(&g_log_wait_queue);
    //     while (entry_idx != LLQUEUE_NULL_IDX) {
    //         // 生成时间戳
    //         std::string timestamp;
    //         LogSyslogPrefixTimestamp(log_settings, timestamp);
    //         // 引入随机延迟
    //         RandomSleep();
    //         // 写入日志信息
    //         WriteToFd(STDERR_FILENO, timestamp.data(), timestamp.size());
    //         // 写入后续的日志信息
    //         std::string *log_str = static_cast<std::string
    //         *>(g_log_queue_entries[entry_idx].data); WriteToFd(STDERR_FILENO, log_str->data(),
    //         log_str->size());
    //         // 释放资源
    //         delete log_str;
    //         llqueue_enqueue(&g_log_free_queue, entry_idx);
    //         entry_idx = llqueue_dequeue_all(&g_log_wait_queue);
    //     }
    // }

    if (ShouldLogToStderr(severity_)) {
        std::lock_guard< std::mutex > lock(g_log_mutex);
        // 生成时间戳
        std::string timestamp;
        LogSyslogPrefixTimestamp(log_settings, timestamp);
        RandomSleep();
        // 写入日志信息
        WriteToFd(STDERR_FILENO, timestamp.data(), timestamp.size());
        WriteToFd(STDERR_FILENO, str_newline.data(), str_newline.size());
    }

    // if ((log_settings.log_dest & LOG_TO_FILE) != 0) {
    //     // If the client app did not call InitLogging() and the lock has not
    //     // been created it will be done now on calling GetLoggingLock(). We do this
    //     // on demand, but if two threads try to do this at the same time, there will
    //     // be a race condition to create the lock. This is why InitLogging should be
    //     // called from the main thread at the beginning of execution.

    //     // base::AutoLock guard(GetLoggingLock());
    //     // if (InitializeLogFileHandle()) {
    //     //     fwrite(str_newline.data(), str_newline.size(), 1, log_settings.log_file);
    //     //     fflush(log_settings.log_file);
    //     // }
    // }
}

// writes the common header info to the stream
void LogMessage::Init(const char *file, const char *func, int line)
{
    static bool g_log_init = false;

    // 初始化日志队列
    if (UNLIKELY(!g_log_init)) {
        g_log_init = true;
        InitLoggingQueue();
    }

    // Don't let actions from this method affect the system error after returning.
    ScopedClearLastError scoped_clear_last_error;

    // 强制只打印文件名字, 不包含路径, 这样信息比较简洁
    file_ = strrchr(file, '/') + 1;
    // 生成日志前缀
    InitWithSyslogPrefix(log_settings);
    // 记录日志信息起始位置
    message_start_ = stream_.str().length();
}

void LogMessage::HandleFatal(size_t stack_start, const std::string &str_newline) const
{
    // char str_stack[1024];
    // std::strncpy(str_stack, str_newline.data(), sizeof(str_stack));

    std::cout << "!!!Self-Abort!!!" << str_newline << std::endl;

    // warn: 总是正常退出, 不会立即崩溃, 这样valgrind测试不会报出大量错误
    exit(-1);

#if defined(__clang__) || defined(__GNUC__)
    __builtin_unreachable();
#endif    // defined(__clang__) || defined(__GNUC__)
}

}    // namespace logging
