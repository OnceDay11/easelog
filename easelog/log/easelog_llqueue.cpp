/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 Once Day <once_day@qq.com>, All rights reserved.
 *
 * @FilePath: /easelog/log/easelog_llqueue.cpp
 * @Author: Once Day <once_day@qq.com>.
 * @Date: 2024-10-10 20:42
 * @info: Encoder=utf-8,Tabsize=4,Eol=\n.
 *
 * @Description:
 *  一个简单的无锁队列实现.
 *
 */

#include "log/easelog_llqueue.h"

namespace logging {

void llqueue_init(struct llqueue *q, struct llqueue_entry *entries, uint32_t entries_sz)
{
    static_assert(sizeof(union llqueue_head) == sizeof(uint64_t), "llqueue_head size error");
    q->head.head   = LLQUEUE_NULL_IDX;
    q->head.cookie = 0;
    q->entries     = entries;
    q->entries_sz  = entries_sz;
    std::atomic_init(&(q->entries_num), 0);
}

uint32_t llqueue_enqueue(struct llqueue *q, uint32_t idx)
{
    union llqueue_head cur_head;
    union llqueue_head new_head;
    bool               ret;

    cur_head.u64 = q->head.u64;
    do {
        new_head.u64         = cur_head.u64;
        q->entries[idx].next = new_head.head;
        new_head.head        = idx;
        new_head.cookie++;
        // 这里同时保证原子操作和内存顺序, 无需额外的内存屏障
        ret = std::atomic_compare_exchange_strong(&(q->head.atomic_u64), &(cur_head.u64),
            new_head.u64);
    } while (ret != true);

    std::atomic_fetch_add(&(q->entries_num), 1);
    return cur_head.head;
}

uint32_t llqueue_dequeue(struct llqueue *q)
{
    union llqueue_head cur_head;
    union llqueue_head new_head;
    bool               ret;

    cur_head.u64 = q->head.u64;
    do {
        if (cur_head.head == LLQUEUE_NULL_IDX) {
            return LLQUEUE_NULL_IDX;
        }
        new_head.u64  = cur_head.u64;
        new_head.head = q->entries[new_head.head].next;
        new_head.cookie++;
        ret = std::atomic_compare_exchange_strong(&(q->head.atomic_u64), &(cur_head.u64),
            new_head.u64);
    } while (ret != true);

    std::atomic_fetch_sub(&(q->entries_num), 1);
    q->entries[cur_head.head].next = LLQUEUE_NULL_IDX;
    return cur_head.head;
}

uint32_t llqueue_dequeue_all(struct llqueue *q)
{
    union llqueue_head empty;
    union llqueue_head cur;
    uint32_t           idx, next, last, count;
    bool               ret;

    empty.head   = LLQUEUE_NULL_IDX;
    empty.cookie = 0;
    cur.u64      = q->head.u64;
    do {
        if (cur.head == LLQUEUE_NULL_IDX) {
            return LLQUEUE_NULL_IDX;
        }
        empty.cookie = cur.cookie + 1u;
        ret = std::atomic_compare_exchange_strong(&(q->head.atomic_u64), &(cur.u64), empty.u64);
    } while (ret != true);

    // 反转顺序, 按照FIFO排序
    next  = cur.head;
    last  = LLQUEUE_NULL_IDX;
    count = 0;
    while (next != LLQUEUE_NULL_IDX) {
        /* dequeue from prev_head */
        idx  = next;
        next = q->entries[idx].next;
        /* enqueue in new_head */
        q->entries[idx].next = last;
        last                 = idx;
        count++;
    }

    std::atomic_fetch_sub(&(q->entries_num), count);
    return last;
}

}    // namespace logging
