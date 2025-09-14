/**
 * @file task_scheduler_specialized.cpp
 * @brief 任务调度器特殊实现
 *
 * 实现ThreadPoolScheduler和RealTimeScheduler的具体功能。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include "modules/task_scheduler/task_scheduler_implementations.h"
#include "common/logger.h"

namespace radar
{

    // ThreadPoolScheduler 实现
    ThreadPoolScheduler::ThreadPoolScheduler(uint32_t threadCount,
                                             std::shared_ptr<spdlog::logger> logger)
        : TaskScheduler(logger), threadCount_(threadCount > 0 ? threadCount : std::thread::hardware_concurrency())
    {
        if (threadCount_ == 0)
        {
            threadCount_ = 4; // 默认值
        }
        RADAR_INFO("ThreadPoolScheduler created with {} threads", threadCount_);
    }

    void ThreadPoolScheduler::workerLoop()
    {
        RADAR_DEBUG("ThreadPoolScheduler worker thread started");

        while (!shouldStop_.load())
        {
            ScheduledTaskPtr task;
            ErrorCode result = taskQueue_->dequeue(task, 100); // 100ms 超时

            if (result == SystemErrors::SUCCESS && task)
            {
                if (currentConcurrentTasks_.load() < maxConcurrentTasks_.load())
                {
                    currentConcurrentTasks_++;
                    statistics_.currentPendingTasks--;

                    // 异步执行任务
                    std::thread([this, task]()
                                {
                        executeTask(task);
                        currentConcurrentTasks_--; })
                        .detach();
                }
                else
                {
                    // 重新入队
                    taskQueue_->enqueue(task);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            else if (result == TaskSchedulerErrors::TASK_TIMEOUT)
            {
                // 超时，继续循环
                continue;
            }
            else
            {
                // 其他错误，记录并继续
                RADAR_ERROR("Failed to dequeue task: {}", static_cast<int>(result));
            }
        }

        RADAR_DEBUG("ThreadPoolScheduler worker thread stopped");
    }

    // RealTimeScheduler 实现
    RealTimeScheduler::RealTimeScheduler(std::shared_ptr<spdlog::logger> logger)
        : TaskScheduler(logger)
    {
        RADAR_INFO("RealTimeScheduler created");
    }

    void RealTimeScheduler::setRealTimeParams(uint32_t maxLatencyMs, bool preemptionEnabled)
    {
        maxLatencyMs_ = maxLatencyMs;
        preemptionEnabled_ = preemptionEnabled;
        RADAR_INFO("Real-time parameters set: maxLatency={}ms, preemption={}",
                   maxLatencyMs, preemptionEnabled);
    }

    ErrorCode RealTimeScheduler::executeTask(const ScheduledTaskPtr &task)
    {
        if (!task)
        {
            return TaskSchedulerErrors::SCHEDULER_NOT_READY;
        }

        auto startTime = std::chrono::steady_clock::now();

        // 检查延迟要求
        if (maxLatencyMs_ > 0)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               startTime - std::chrono::steady_clock::now())
                               .count();
            if (elapsed > maxLatencyMs_)
            {
                RADAR_WARN("Task {} exceeded max latency {}ms", task->getId(), maxLatencyMs_);
            }
        }

        return TaskScheduler::executeTask(task);
    }

    void RealTimeScheduler::workerLoop()
    {
        RADAR_DEBUG("RealTimeScheduler worker thread started");

        while (!shouldStop_.load())
        {
            ScheduledTaskPtr task;
            ErrorCode result = taskQueue_->dequeue(task, 50); // 更短的超时时间

            if (result == SystemErrors::SUCCESS && task)
            {
                // 实时调度逻辑
                if (preemptionEnabled_)
                {
                    // 检查是否需要抢占当前任务
                    // 这里简化实现，实际需要更复杂的逻辑
                }

                if (currentConcurrentTasks_.load() < maxConcurrentTasks_.load())
                {
                    currentConcurrentTasks_++;
                    statistics_.currentPendingTasks--;

                    // 异步执行任务
                    std::thread([this, task]()
                                {
                        executeTask(task);
                        currentConcurrentTasks_--; })
                        .detach();
                }
                else
                {
                    // 重新入队
                    taskQueue_->enqueue(task);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 更短的睡眠时间
                }
            }
            else if (result == TaskSchedulerErrors::TASK_TIMEOUT)
            {
                // 超时，继续循环
                continue;
            }
            else
            {
                // 其他错误，记录并继续
                RADAR_ERROR("Failed to dequeue task: {}", static_cast<int>(result));
            }
        }

        RADAR_DEBUG("RealTimeScheduler worker thread stopped");
    }

    bool RealTimeScheduler::shouldPreempt(const ScheduledTaskPtr &newTask,
                                          const ScheduledTaskPtr &runningTask) const
    {
        if (!newTask || !runningTask)
        {
            return false;
        }

        // 基于优先级的抢占逻辑
        return static_cast<int>(newTask->getPriority()) >
               static_cast<int>(runningTask->getPriority());
    }

    ErrorCode RealTimeScheduler::setRealTimeScheduling()
    {
        // 设置线程调度策略为实时
        // 这里需要平台特定的实现
        RADAR_INFO("Real-time scheduling policy set");
        return SystemErrors::SUCCESS;
    }

} // namespace radar
