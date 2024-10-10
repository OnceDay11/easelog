/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog_llqueue.h
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-10 20:42
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  一个简单的无锁队列实现, 数据结构和函数声明.
 *
 */

#ifndef EASELOG_LLQUEUE_H
#define EASELOG_LLQUEUE_H

#include <stdint.h>
#include <atomic>

namespace logging {

// Lockless queue, allowing concurrent enqueues/dequeues.
//
// Example of use for a lock less FIFO using two queues, one having the free
// elements and the second one having the messages.
//
//  union llqueue_t message_queue;
//  union llqueue_t free_elements;
//  struct easeipc_llqueue_t elements[512];
//  uint16_t idx;
//  uint16_t next;
//
// * Init. *
// easeipc_llqueue_init(&message_queue, elements, 512);
// easeipc_llqueue_init(&free_elements, elements, 512);
// for (i = 0; i != 512; ++i)
//    easeipc_llqueue_enqueue(&free_elements, i);
//
//
//  Add an entry to the message queue, by allocating a memory space from the
//  free_elements queue
//
//  idx = easeipc_llqueue_dequeue(&free_elements);
//  elements[idx].data = mydata;
//  easeipc_llqueue_enqueue(&message_queue, idx);
//
//
//  process all entries from the message queue and "free" the memory by
//  enqueueing on the free_elements queue.
//
// idx = easeipc_llqueue_dequeue_all(&message_queue);
// idx = easeipc_llqueue_reverse(&message_queue, idx);
// while (idx != EASEIPC_LLQUEUE_NULL_IDX) {
//    process(elements[idx].data);
//    next = elements[idx].next;
//    easeipc_llqueue_enqueue(&free_elements, idx);
//    idx = next;
// }

// 标识无效索引
#define LLQUEUE_NULL_IDX (0xffffffff)

// 无锁队列元素结构体
struct llqueue_entry {
    uint32_t next;     // 下一个元素索引
    uint32_t __pad;    // 保留字段
    void    *data;     // 数据指针
};

// 队列头部索引
union llqueue_head {
    struct {
        uint32_t head;      // 队列头索引
        uint32_t cookie;    // 队列cookie, 防止ABA问题
    };

    uint64_t             u64;           // 用于普通操作
    std::atomic_uint64_t atomic_u64;    // 用于原子操作
};

// 无锁队列结构体
struct llqueue {
    union llqueue_head    head;           // 队列头部
    uint32_t              entries_sz;     // 队列元素个数
    std::atomic_uint32_t  entries_num;    // 队列元素个数
    struct llqueue_entry *entries;        // 队列元素数组
};

void llqueue_init(struct llqueue *q, struct llqueue_entry *entries, uint32_t entries_sz);

uint32_t llqueue_enqueue(struct llqueue *q, uint32_t idx);

uint32_t llqueue_dequeue(struct llqueue *q);

uint32_t llqueue_dequeue_all(struct llqueue *q);

}    // namespace logging
#endif    // EASELOG_LLQUEUE_H
