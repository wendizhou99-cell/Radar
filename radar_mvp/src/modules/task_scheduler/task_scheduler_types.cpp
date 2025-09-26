/**
 * @file task_scheduler_types.cpp
 * @brief 任务调度器数据类型实现
 *
 * 实现任务调度器模块中使用的数据类型和结构体。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include "modules/task_scheduler/task_scheduler_types.h"

#include <chrono>

#include "common/error_codes.h"
#include "common/logger.h"

namespace radar {

// 静态成员初始化
std::atomic<ScheduledTask::TaskId> ScheduledTask::nextTaskId_(1);

ScheduledTask::ScheduledTask(TaskFunction taskFunction, PacketPriority priority, uint32_t timeoutMs,
                             const std::string &name)
    : name_(name.empty() ? "Task_" + std::to_string(nextTaskId_.load()) : name),
      taskFunction_(std::move(taskFunction)),
      priority_(priority),
      state_(TaskState::PENDING),
      timeoutMs_(timeoutMs),
      submitTime_(std::chrono::system_clock::now()) {
    taskId_ = nextTaskId_.fetch_add(1);
    RADAR_INFO("Created task {} with priority {}", taskId_, static_cast<int>(priority_));
}

ErrorCode ScheduledTask::execute() {
    if (!taskFunction_) {
        RADAR_ERROR("Task {} has no valid function to execute", taskId_);
        return TaskSchedulerErrors::SCHEDULING_ERROR;
    }

    setState(TaskState::RUNNING);
    startTime_ = std::chrono::system_clock::now();

    try {
        RADAR_DEBUG("Executing task {}", taskId_);
        taskFunction_();
        finishTime_ = std::chrono::system_clock::now();
        setState(TaskState::COMPLETED);
        RADAR_INFO("Task {} completed successfully", taskId_);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        finishTime_ = std::chrono::system_clock::now();
        setState(TaskState::FAILED);
        RADAR_ERROR("Task {} failed with exception: {}", taskId_, e.what());
        return TaskSchedulerErrors::TASK_EXECUTION_FAILED;
    } catch (...) {
        finishTime_ = std::chrono::system_clock::now();
        setState(TaskState::FAILED);
        RADAR_ERROR("Task {} failed with unknown exception", taskId_);
        return TaskSchedulerErrors::TASK_EXECUTION_FAILED;
    }
}

ErrorCode ScheduledTask::cancel() {
    TaskState expected = TaskState::PENDING;
    if (state_.compare_exchange_strong(expected, TaskState::CANCELLED)) {
        finishTime_ = std::chrono::system_clock::now();
        RADAR_INFO("Task {} cancelled", taskId_);
        return SystemErrors::SUCCESS;
    }

    RADAR_WARN("Cannot cancel task {} in state {}", taskId_, static_cast<int>(state_.load()));
    return TaskSchedulerErrors::SCHEDULING_ERROR;
}

double ScheduledTask::getExecutionTimeMs() const {
    if (startTime_ == std::chrono::system_clock::time_point{} ||
        finishTime_ == std::chrono::system_clock::time_point{}) {
        return 0.0;
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finishTime_ - startTime_);
    return static_cast<double>(duration.count());
}

double ScheduledTask::getWaitingTimeMs() const {
    if (startTime_ == std::chrono::system_clock::time_point{}) {
        return 0.0;
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime_ - submitTime_);
    return static_cast<double>(duration.count());
}

bool ScheduledTask::isTimeout() const {
    if (timeoutMs_ == 0) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - submitTime_);
    return elapsed.count() > timeoutMs_;
}

bool ScheduledTask::operator<(const ScheduledTask &other) const {
    // 优先级数值越大，优先级越高
    return static_cast<int>(priority_) < static_cast<int>(other.priority_);
}

void ScheduledTask::setState(TaskState newState) {
    TaskState oldState = state_.load();
    state_.store(newState);
    RADAR_DEBUG("Task {} state changed from {} to {}", taskId_, static_cast<int>(oldState), static_cast<int>(newState));

    // 使用oldState避免警告
    (void)oldState;
}

void TaskStatistics::updateExecutionStats(double executionTimeMs, double waitingTimeMs) {
    // 更新平均执行时间
    uint64_t completed = totalTasksCompleted.load();
    if (completed > 0) {
        double currentAvg = averageExecutionTimeMs.load();
        averageExecutionTimeMs.store((currentAvg * (completed - 1) + executionTimeMs) / completed);
    } else {
        averageExecutionTimeMs.store(executionTimeMs);
    }

    // 更新平均等待时间
    if (completed > 0) {
        double currentAvg = averageWaitingTimeMs.load();
        averageWaitingTimeMs.store((currentAvg * (completed - 1) + waitingTimeMs) / completed);
    } else {
        averageWaitingTimeMs.store(waitingTimeMs);
    }

    // 更新吞吐量（每秒任务数）
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
    if (elapsed > 0) {
        throughputTasksPerSecond.store(static_cast<double>(completed) / elapsed);
    }

    lastUpdateTime_ = now;
}

}  // namespace radar
