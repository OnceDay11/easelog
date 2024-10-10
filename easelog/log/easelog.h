/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog.h
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-08-17 14:33
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  实现程序Log的数据类型和方法定义.
 *
 */

#ifndef EASELOG_LOGGING_H_
#define EASELOG_LOGGING_H_

#include <stdint.h>

#include <sstream>

namespace logging {

// A bitmask of potential logging destinations.
using LoggingDestination = uint32_t;

// Specifies where logs will be written. Multiple destinations can be specified
// with bitwise OR.
// Unless destination is LOG_NONE, all logs with severity ERROR and above will
// be written to stderr in addition to the specified destination.
// LOG_TO_FILE includes logging to externally-provided file handles.
enum : uint32_t {
    LOG_NONE                = 0,
    LOG_TO_FILE             = 1 << 0,
    LOG_TO_SYSTEM_DEBUG_LOG = 1 << 1,
    LOG_TO_STDERR           = 1 << 2,

    LOG_TO_ALL = LOG_TO_FILE | LOG_TO_SYSTEM_DEBUG_LOG | LOG_TO_STDERR,

    // On POSIX platforms, where it may not even be possible to locate the
    // executable on disk, use stderr.
    LOG_DEFAULT = LOG_TO_SYSTEM_DEBUG_LOG | LOG_TO_STDERR,
};

using LogSeverity = int32_t;
// This is level 1 verbosity
// Note: the log severities are used to index into the array of names,
// see log_severity_names.
constexpr LogSeverity LOGGING_DEBUG          = 0;
constexpr LogSeverity LOGGING_INFO           = 1;
constexpr LogSeverity LOGGING_WARNING        = 2;
constexpr LogSeverity LOGGING_ERROR          = 3;
constexpr LogSeverity LOGGING_FATAL          = 4;
constexpr LogSeverity LOGGING_NUM_SEVERITIES = 5;

using FilePath   = std::string;
using FileHandle = FILE *;

struct LoggingSettings {
    // The path to the log file.
    FilePath   *log_file_path;
    // The file handle for the log file.
    FileHandle  log_file;
    // The minimum log level to output.
    int32_t     log_min_level;
    // For LOGGING_ERROR and above, always print to stderr.
    int32_t     log_always_print;
    // Specifies the process' logging sink(s), represented as a combination of
    // LoggingDestination values joined by bitwise OR.
    // The destination for the log messages.
    uint32_t    log_dest;
    // What should be prepended to each message?
    bool        log_process_id;
    bool        log_thread_id;
    bool        log_timestamp;
    bool        log_tickcount;
    const char *log_prefix;
    // The names of the log severities.
    const char *log_severity_names[LOGGING_NUM_SEVERITIES];
};

// Returns the logging settings.
const struct LoggingSettings &GetLoggingSettings();

// Init the logging settings.
bool BaseInitLoggingImpl(const LoggingSettings &settings);

// Sets the log file name and other global logging state. Calling this function
// is recommended, and is normally done at the beginning of application init.
// If you don't call it, all the flags will be initialized to their default
// values, and there is a race condition that may leak a critical section
// object if two threads try to do the first log at the same time.
// See the definition of the enums above for descriptions and default values.
//
// The default log file is initialized to "debug.log" in the application
// directory. You probably don't want this, especially since the program
// directory may not be writable on an enduser's system.
//
// This function may be called a second time to re-direct logging (e.g after
// loging in to a user partition), however it should never be called more than
// twice.
inline bool InitLogging(const LoggingSettings &settings)
{
    return BaseInitLoggingImpl(settings);
}

// Initializes logging with the default settings.
inline bool InitLogging(void)
{
    return BaseInitLoggingImpl(GetLoggingSettings());
}

// Sets the log level. Anything at or above this level will be written to the
// log file/displayed to the user (if applicable). Anything below this level
// will be silently ignored. The log level defaults to 0 (everything is logged
// up to level INFO) if this function is not called.
// Note that log messages for VLOG(x) are logged at level -x, so setting
// the min log level to negative values enables verbose logging and conversely,
// setting the VLOG default level will set this min level to a negative number,
// effectively enabling all levels of logging.
void SetMinLogLevel(int32_t level);

// Gets the current log level.
int32_t GetMinLogLevel();

// Used by LOG_IS_ON to lazy-evaluate stream arguments.
bool ShouldCreateLogMessage(int32_t severity);

// Sets the common items you want to be prepended to each log message.
// process and thread IDs default to off, the timestamp defaults to on.
// If this function is not called, logging defaults to writing the timestamp
// only.
void SetLogItems(bool enable_process_id, bool enable_thread_id, bool enable_timestamp,
    bool enable_tickcount);

// Sets an optional prefix to add to each log message. |prefix| is not copied
// and should be a raw string constant. |prefix| must only contain ASCII letters
// to avoid confusion with PIDs and timestamps. Pass null to remove the prefix.
// Logging defaults to no prefix.
void SetLogPrefix(const char *prefix);

// Generates a timestamp string for the log message.
void LogSyslogPrefixTimestamp(const LoggingSettings &log_settings, std::string &timestamp);

// This class more or less represents a particular log message.  You
// create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to the appropriate destination.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though.  You should use the LOG() macro (and variants thereof)
// above.
class LogMessage {
public:
    // Used for LOG(severity).
    LogMessage(const char *file, const char *func, int line, LogSeverity severity);

    // Used for CHECK().  Implied severity = LOGGING_FATAL.
    LogMessage(const char *file, const char *func, int line, const char *condition);

    // Delete copy constructor and assignment operator.
    LogMessage(const LogMessage &)            = delete;
    LogMessage &operator=(const LogMessage &) = delete;
    virtual ~LogMessage();

    std::ostream &stream() { return stream_; }

    LogSeverity severity() const { return severity_; }

    std::string str() const { return stream_.str(); }

    const char *file() const { return file_; }

    const char *func() const { return func_; }

    int line() const { return line_; }

protected:
    void Flush();

private:
    void Init(const char *file, const char *func, int line);
    void InitWithSyslogPrefix(const LoggingSettings &settings);

    void HandleFatal(size_t stack_start, const std::string &str_newline) const;

    std::ostringstream stream_;
    // Offset of the start of the message (past prefix info).
    size_t             message_start_;
    // The file and line information passed in to the constructor.
    const char        *file_;
    const char        *func_;
    const int32_t      line_;
    const LogSeverity  severity_;
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
public:
    LogMessageVoidify() = default;

    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(std::ostream &) { }
};

// Helper macro which avoids evaluating the arguments to a stream if
// the condition doesn't hold. Condition is evaluated once and only once.
#define LAZY_STREAM(stream, condition) \
    !(condition) ? (void)0 : ::logging::LogMessageVoidify() & (stream)

// As special cases, we can assume that LOG_IS_ON(FATAL) always holds. Also,
// LOG_IS_ON(DFATAL) always holds in debug mode. In particular, CHECK()s will
// always fire if they fail.
// FATAL is always enabled and required to be resolved in compile time for
// LOG(FATAL) to be properly understood as [[noreturn]].
#define LOG_IS_ON(severity) (::logging::ShouldCreateLogMessage(::logging::LOGGING_##severity))

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token COMPACT_LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.
#define LOG_STREAM(severity) \
    ::logging::LogMessage(__FILE__, __func__, __LINE__, ::logging::LOGGING_##severity).stream()
// 基础日志类型, 直接日志输出.
#define LOG(severity) LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity))
// 拓展日志类型, 简单条件日志输出.
#define LOG_IF(severity, condition) \
    LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity) && (condition))

}    // namespace logging

#endif    // EASELOG_LOGGING_H_
