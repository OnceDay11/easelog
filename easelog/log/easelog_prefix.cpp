/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog_prefix.cpp
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-09 22:46
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  实现日志前缀信息格式化功能
 *
 */

// Define _GNU_SOURCE to ensure that <errno.h> defines
// program_invocation_short_name which is used in GetProgramName(). Keep this at
// the top of the file since some system headers might include <errno.h> and the
// header could be skipped on subsequent includes.
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "log/easelog.h"
#include "log/easelog_private.h"

#include <errno.h>
#include <iomanip>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

namespace logging {

// On POSIX, our ProcessId will just be the PID.
using ProcessId                    = pid_t;
constexpr ProcessId kNullProcessId = 0;

// Get the process ID of the current process.
static inline ProcessId GetCurrentProcessId()
{
    // This is a static function local variable that is used to store the PID of the process.
    static ProcessId g_process_pid = kNullProcessId;

    if (UNLIKELY(g_process_pid == kNullProcessId)) {
        g_process_pid = getpid();
    }
    return g_process_pid;
}

// Get the program name of the current process.
static inline const char *GetProgramName(void)
{
    return program_invocation_short_name ? program_invocation_short_name : "(unknown)";
}

// Get the process ID of the current thread.
static inline ProcessId GetCurrentThreadId(void)
{
    static thread_local ProcessId g_thread_tid = kNullProcessId;
    decltype(syscall(0))          ret;

    if (g_thread_tid == kNullProcessId) {
        ret          = syscall(__NR_gettid);
        g_thread_tid = static_cast< ProcessId >(ret);
    }
    return g_thread_tid;
}

// 获取日志服务等级名称, 输出C字符串指针
static const char *log_severity_name(const LoggingSettings &log_settings, int32_t severity)
{
    if (LIKELY(severity >= 0 && severity < LOGGING_NUM_SEVERITIES)) {
        return log_settings.log_severity_names[severity];
    } else if (severity < 0) {
        return "VERBOSE";
    }
    return "Unknown";
}

void LogSyslogPrefixTimestamp(const LoggingSettings &log_settings, std::string &timestamp)
{
    std::ostringstream stream;

    if (log_settings.log_timestamp) {
        timeval tv{};
        gettimeofday(&tv, nullptr);
        time_t t = tv.tv_sec;

        struct tm utc_time { };

        localtime_r(&t, &utc_time);
        stream << std::setfill('0')                                          // Set fill to 0
               << std::setw(4) << 1900 + utc_time.tm_year << '-'             // year
               << std::setw(2) << 1 + utc_time.tm_mon << '-'                 // month
               << std::setw(2) << utc_time.tm_mday                           // date
               << 'T' << std::setw(2) << utc_time.tm_hour << ':'             // hour
               << std::setw(2) << utc_time.tm_min << ':'                     // minute
               << std::setw(2) << utc_time.tm_sec << '.'                     // second
               << std::setw(6) << tv.tv_usec                                 // millisecond
               << '+' << std::setw(2) << utc_time.tm_gmtoff / 3600 << ':'    // timezone hour
               << std::setw(2) << utc_time.tm_gmtoff % 3600 << ' ';          // timezone second
    }

    timestamp = stream.str();
}

// base style log prefix, eg.
// [unknown_pid:unknown_tid:0826/145119.098911:19408886280525:info:logging_unittest.cpp(66)]
// log message
static void InitSyslogPrefixWithBaseStyle(LogMessage &log, const LoggingSettings &log_settings)
{
    std::ostream &stream_ = log.stream();

    if (log_settings.log_prefix) {
        stream_ << log_settings.log_prefix << ':';
    }
    if (log_settings.log_tickcount) {
        stream_ << TickCountUs() << ' ';
    }

    // 日志等级信息
    stream_ << '<' << log_severity_name(log_settings, log.severity());
    if (log.severity() < 0) {
        stream_ << -log.severity();
    }
    stream_ << '>';

    stream_ << ' ' << GetProgramName();
    if (log_settings.log_process_id) {
        stream_ << '[' << GetCurrentProcessId() << ']';
    }
    stream_ << ": [";
    if (log_settings.log_thread_id) {
        char thread_name[16] = {0};
        // 获取当前线程的名字
        pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name));
        stream_ << thread_name << '(' << GetCurrentThreadId() << ") - ";
    }
    stream_ << log.file() << '(' << log.func() << '-' << log.line() << ")] ";
}

void LogMessage::InitWithSyslogPrefix(const LoggingSettings &settings)
{
    InitSyslogPrefixWithBaseStyle(*this, settings);
}

}    // namespace logging
