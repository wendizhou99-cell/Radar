/**
 * @file task_scheduler_queues.cpp
 * @brief 任务调度器队列实现
 *
 * 实现任务调度器模块中的各种队列类。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include <chrono>

#include "common/logger.h"
#include "modules/task_scheduler/task_scheduler_implementations.h"

namespace radar {

// FIFOTaskQueue 实现
ErrorCode FIFOTaskQueue::enqueue(const ScheduledTaskPtr &task) {
    if (!task) {
        RADAR_ERROR("Cannot enqueue null task");
        return TaskSchedulerErrors::SCHEDULING_ERROR;
    }

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    taskAvailable_.notify_one();

    RADAR_DEBUG("Enqueued task {} to FIFO queue", task->getId());
    return SystemErrors::SUCCESS;
}

ErrorCode FIFOTaskQueue::dequeue(ScheduledTaskPtr &task, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (timeoutMs == 0) {
        // 非阻塞模式
        if (taskQueue_.empty()) {
            return TaskSchedulerErrors::TASK_QUEUE_FULL;
        }
        task = taskQueue_.front();
        taskQueue_.pop();
        return SystemErrors::SUCCESS;
    }

    // 阻塞模式，支持超时
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (taskAvailable_.wait_for(lock, timeout, [this]() { return !taskQueue_.empty(); })) {
        if (!taskQueue_.empty()) {
            task = taskQueue_.front();
            taskQueue_.pop();
            RADAR_DEBUG("Dequeued task {} from FIFO queue", task->getId());
            return SystemErrors::SUCCESS;
        }
    }

    RADAR_DEBUG("Timeout waiting for task in FIFO queue");
    return TaskSchedulerErrors::TASK_TIMEOUT;
}

size_t FIFOTaskQueue::size() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

bool FIFOTaskQueue::empty() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return taskQueue_.empty();
}

void FIFOTaskQueue::clear() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    while (!taskQueue_.empty()) {
        taskQueue_.pop();
    }
    RADAR_INFO("FIFO queue cleared");
}

// PriorityTaskQueue 实现
ErrorCode PriorityTaskQueue::enqueue(const ScheduledTaskPtr &task) {
    if (!task) {
        RADAR_ERROR("Cannot enqueue null task");
        return TaskSchedulerErrors::SCHEDULING_ERROR;
    }

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    taskAvailable_.notify_one();

    RADAR_DEBUG("Enqueued task {} to priority queue", task->getId());
    return SystemErrors::SUCCESS;
}

ErrorCode PriorityTaskQueue::dequeue(ScheduledTaskPtr &task, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(queueMutex_);

    if (timeoutMs == 0) {
        // 非阻塞模式
        if (taskQueue_.empty()) {
            return TaskSchedulerErrors::TASK_QUEUE_FULL;
        }
        task = taskQueue_.top();
        taskQueue_.pop();
        return SystemErrors::SUCCESS;
    }

    // 阻塞模式，支持超时
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (taskAvailable_.wait_for(lock, timeout, [this]() { return !taskQueue_.empty(); })) {
        if (!taskQueue_.empty()) {
            task = taskQueue_.top();
            taskQueue_.pop();
            RADAR_DEBUG("Dequeued task {} from priority queue", task->getId());
            return SystemErrors::SUCCESS;
        }
    }

    RADAR_DEBUG("Timeout waiting for task in priority queue");
    return TaskSchedulerErrors::TASK_TIMEOUT;
}

size_t PriorityTaskQueue::size() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

bool PriorityTaskQueue::empty() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return taskQueue_.empty();
}

void PriorityTaskQueue::clear() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    while (!taskQueue_.empty()) {
        taskQueue_.pop();
    }
    RADAR_INFO("Priority queue cleared");
}

}  // namespace radar
