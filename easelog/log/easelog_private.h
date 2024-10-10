/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog_private.h
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-09 22:49
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  定义EaseLog的私有数据类型和方法.
 *
 */

#ifndef EASELOG_PRIVATE_H_
#define EASELOG_PRIVATE_H_

#include <errno.h>

#include <utility>
#include <type_traits>
#include <functional>

namespace logging {

// This provides a wrapper around system calls which may be interrupted by a
// signal and return EINTR. See man 7 signal.
// To prevent long-lasting loops (which would likely be a bug, such as a signal
// that should be masked) to go unnoticed, there is a limit after which the
// caller will nonetheless see an EINTR in Debug builds.
#if defined(NDEBUG)

#define HANDLE_EINTR(ret, func_call)                            \
    do {                                                        \
        decltype(func_call) eintr_wrapper_result;               \
        do {                                                    \
            eintr_wrapper_result = (func_call);                 \
        } while (eintr_wrapper_result == -1 && errno == EINTR); \
        eintr_wrapper_result;                                   \
    } while (0)

#else

#define HANDLE_EINTR(ret, func_call)                                                             \
    do {                                                                                         \
        int                 eintr_wrapper_counter = 0;                                           \
        decltype(func_call) eintr_wrapper_result;                                                \
        do {                                                                                     \
            eintr_wrapper_result = (func_call);                                                  \
        } while (eintr_wrapper_result == -1 && errno == EINTR && eintr_wrapper_counter++ < 100); \
        ret = eintr_wrapper_result;                                                              \
    } while (0)

#endif    // NDEBUG

// Macro for hinting that an expression is likely to be false.
#if !defined(UNLIKELY)
#if defined(COMPILER_GCC) || defined(__clang__)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif    // defined(COMPILER_GCC)
#endif    // !defined(UNLIKELY)

#if !defined(LIKELY)
#if defined(COMPILER_GCC) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define LIKELY(x) (x)
#endif    // defined(COMPILER_GCC)
#endif    // !defined(LIKELY)

// ScopedClearLastError stores and resets the value of thread local error codes
// (errno, GetLastError()), and restores them in the destructor. This is useful
// to avoid side effects on these values in instrumentation functions that
// interact with the OS.
class ScopedClearLastError {
public:
    ScopedClearLastError() : last_errno_(errno) { errno = 0; }

    ScopedClearLastError(const ScopedClearLastError &)            = delete;
    ScopedClearLastError &operator=(const ScopedClearLastError &) = delete;

    ~ScopedClearLastError() { errno = last_errno_; }

private:
    const int last_errno_;
};

// ScopedCleanUp is a helper class that calls a given function when it goes out of
// scope. This is useful for cleanup code that needs to run regardless of how the
// current scope is exited.
// A generic RAII type that runs a user-provided function in its destructor.
class ScopedCleanUp final {
public:
    explicit ScopedCleanUp(std::function< void() > f) : f_(std::move(f)) { }

    ScopedCleanUp(const ScopedCleanUp &)            = delete;
    ScopedCleanUp &operator=(const ScopedCleanUp &) = delete;

    ~ScopedCleanUp() { f_(); }

private:
    std::function< void() > f_;
};

// 获取系统滴答计数, 用于绝对时间戳, 0通常是无效值
static inline uint64_t TickCountUs()
{
    struct timespec ts;
    uint64_t        absolute_us;
    int32_t         ret;

    // clock_gettime()函数已使用VSDO机制, 无需走syscall系统调用路径.
    // 64位无符号数可以存储以ns为单位的时间最长584年, 以ms为单位的时间最长584e6年.
    // 这里返回的是以us为单位的时间戳.
    // CLOCK_MONOTONIC_RAW: 从系统启动这一刻起开始计时, 不受系统时间被用户改变的影响.
    // CLOCK_MONOTONIC: 从系统启动这一刻起开始计时, 不受系统时间被用户改变的影响, 但速度可能改变.
    // CLOCK_MONOTONIC_RAW 不会收到NTP服务的影响, 但也会受到系统的时钟漂移影响.
    ret = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    if (UNLIKELY(ret != 0)) {
        return 0;
    }

    absolute_us =
        static_cast< uint64_t >(ts.tv_sec) * 1000000 + static_cast< uint64_t >(ts.tv_nsec) / 1000;

    return absolute_us;
}

// 用于构造并发时序, 随机等待 10-50ms
void RandomSleep();

}    // namespace logging

#endif    // EASELOG_PRIVATE_H_
