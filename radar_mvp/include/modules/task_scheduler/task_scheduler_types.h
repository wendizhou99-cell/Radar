/**
 * @file task_scheduler_types.h
 * @brief 任务调度器数据类型定义
 *
 * 定义任务调度器模块中使用的数据类型、枚举和结构体。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "common/error_codes.h"
#include "common/types.h"

namespace radar {

/**
 * @brief 任务优先级枚举
 */
enum class TaskPriority : int {
    LOW = 0,       ///< 低优先级
    NORMAL = 1,    ///< 普通优先级
    HIGH = 2,      ///< 高优先级
    CRITICAL = 3,  ///< 关键优先级
    REALTIME = 4   ///< 实时优先级
};

/**
 * @brief 任务状态枚举
 */
enum class TaskState {
    PENDING,    ///< 等待执行
    RUNNING,    ///< 正在执行
    COMPLETED,  ///< 已完成
    FAILED,     ///< 执行失败
    CANCELLED,  ///< 已取消
    TIMEOUT     ///< 超时
};

/**
 * @brief 调度策略枚举
 */
enum class SchedulingStrategy {
    FIFO,                     ///< 先进先出
    PRIORITY,                 ///< 优先级调度
    ROUND_ROBIN,              ///< 时间片轮转
    LOAD_BALANCE,             ///< 负载均衡
    EARLIEST_DEADLINE_FIRST,  ///< 最早截止时间优先
    RATE_MONOTONIC            ///< 速率单调调度
};

/**
 * @brief 内部任务包装类
 *
 * 封装原始的任务函数，添加ID、优先级、状态等管理信息
 */
class ScheduledTask {
  public:
    using TaskId = uint64_t;
    using TaskFunction = std::function<void()>;

    /**
     * @brief 构造函数
     * @param taskFunction 任务执行函数
     * @param priority 任务优先级
     * @param timeoutMs 超时时间（毫秒）
     * @param name 任务名称
     */
    ScheduledTask(TaskFunction taskFunction, PacketPriority priority = PacketPriority::NORMAL, uint32_t timeoutMs = 0,
                  const std::string &name = "");

    /**
     * @brief 析构函数
     */
    virtual ~ScheduledTask() = default;

    /**
     * @brief 执行任务
     * @return 操作结果错误码
     */
    virtual ErrorCode execute();

    /**
     * @brief 取消任务
     * @return 操作结果错误码
     */
    virtual ErrorCode cancel();

    // Getters
    TaskId getId() const {
        return taskId_;
    }
    const std::string &getName() const {
        return name_;
    }
    PacketPriority getPriority() const {
        return priority_;
    }
    TaskState getState() const {
        return state_.load();
    }
    std::chrono::system_clock::time_point getSubmitTime() const {
        return submitTime_;
    }
    std::chrono::system_clock::time_point getStartTime() const {
        return startTime_;
    }
    std::chrono::system_clock::time_point getFinishTime() const {
        return finishTime_;
    }
    uint32_t getTimeoutMs() const {
        return timeoutMs_;
    }

    // Setters
    void setPriority(PacketPriority priority) {
        priority_ = priority;
    }
    void setTimeoutMs(uint32_t timeoutMs) {
        timeoutMs_ = timeoutMs;
    }

    /**
     * @brief 获取执行时间
     * @return 任务执行耗时（毫秒）
     */
    double getExecutionTimeMs() const;

    /**
     * @brief 获取等待时间
     * @return 任务等待耗时（毫秒）
     */
    double getWaitingTimeMs() const;

    /**
     * @brief 检查任务是否超时
     * @return 任务是否已超时
     */
    bool isTimeout() const;

    /**
     * @brief 任务优先级比较（用于优先级队列）
     * @param other 另一个任务
     * @return 当前任务优先级是否低于另一个任务
     */
    bool operator<(const ScheduledTask &other) const;

  protected:
    /**
     * @brief 设置任务状态
     * @param newState 新状态
     */
    void setState(TaskState newState);

  private:
    static std::atomic<TaskId> nextTaskId_;  ///< 全局任务ID计数器

    TaskId taskId_;                 ///< 任务唯一ID
    std::string name_;              ///< 任务名称
    TaskFunction taskFunction_;     ///< 任务执行函数
    PacketPriority priority_;       ///< 任务优先级
    std::atomic<TaskState> state_;  ///< 任务状态
    uint32_t timeoutMs_;            ///< 超时时间（毫秒）

    // 时间戳
    std::chrono::system_clock::time_point submitTime_;  ///< 提交时间
    std::chrono::system_clock::time_point startTime_;   ///< 开始执行时间
    std::chrono::system_clock::time_point finishTime_;  ///< 完成时间
};

using ScheduledTaskPtr = std::shared_ptr<ScheduledTask>;

/**
 * @brief 任务统计信息结构
 */
struct TaskStatistics {
    std::atomic<uint64_t> totalTasksSubmitted{0};       ///< 提交的总任务数
    std::atomic<uint64_t> totalTasksCompleted{0};       ///< 完成的总任务数
    std::atomic<uint64_t> totalTasksFailed{0};          ///< 失败的总任务数
    std::atomic<uint64_t> totalTasksCancelled{0};       ///< 取消的总任务数
    std::atomic<uint64_t> totalTasksTimeout{0};         ///< 超时的总任务数
    std::atomic<uint32_t> currentPendingTasks{0};       ///< 当前等待任务数
    std::atomic<uint32_t> currentRunningTasks{0};       ///< 当前执行任务数
    std::atomic<double> averageExecutionTimeMs{0.0};    ///< 平均执行时间（毫秒）
    std::atomic<double> averageWaitingTimeMs{0.0};      ///< 平均等待时间（毫秒）
    std::atomic<double> throughputTasksPerSecond{0.0};  ///< 吞吐量（任务/秒）

    std::chrono::system_clock::time_point startTime_;       ///< 开始时间
    std::chrono::system_clock::time_point lastUpdateTime_;  ///< 最后更新时间

    // 支持复制构造和赋值
    TaskStatistics() = default;
    TaskStatistics(const TaskStatistics &other) {
        totalTasksSubmitted.store(other.totalTasksSubmitted.load());
        totalTasksCompleted.store(other.totalTasksCompleted.load());
        totalTasksFailed.store(other.totalTasksFailed.load());
        totalTasksCancelled.store(other.totalTasksCancelled.load());
        totalTasksTimeout.store(other.totalTasksTimeout.load());
        currentPendingTasks.store(other.currentPendingTasks.load());
        currentRunningTasks.store(other.currentRunningTasks.load());
        averageExecutionTimeMs.store(other.averageExecutionTimeMs.load());
        averageWaitingTimeMs.store(other.averageWaitingTimeMs.load());
        throughputTasksPerSecond.store(other.throughputTasksPerSecond.load());
        startTime_ = other.startTime_;
        lastUpdateTime_ = other.lastUpdateTime_;
    }
    TaskStatistics &operator=(const TaskStatistics &other) {
        if (this != &other) {
            totalTasksSubmitted.store(other.totalTasksSubmitted.load());
            totalTasksCompleted.store(other.totalTasksCompleted.load());
            totalTasksFailed.store(other.totalTasksFailed.load());
            totalTasksCancelled.store(other.totalTasksCancelled.load());
            totalTasksTimeout.store(other.totalTasksTimeout.load());
            currentPendingTasks.store(other.currentPendingTasks.load());
            currentRunningTasks.store(other.currentRunningTasks.load());
            averageExecutionTimeMs.store(other.averageExecutionTimeMs.load());
            averageWaitingTimeMs.store(other.averageWaitingTimeMs.load());
            throughputTasksPerSecond.store(other.throughputTasksPerSecond.load());
            startTime_ = other.startTime_;
            lastUpdateTime_ = other.lastUpdateTime_;
        }
        return *this;
    }

    /**
     * @brief 重置所有统计信息
     */
    void reset() {
        totalTasksSubmitted = 0;
        totalTasksCompleted = 0;
        totalTasksFailed = 0;
        totalTasksCancelled = 0;
        totalTasksTimeout = 0;
        currentPendingTasks = 0;
        currentRunningTasks = 0;
        averageExecutionTimeMs = 0.0;
        averageWaitingTimeMs = 0.0;
        throughputTasksPerSecond = 0.0;
        startTime_ = std::chrono::system_clock::now();
        lastUpdateTime_ = startTime_;
    }

    /**
     * @brief 更新执行统计
     * @param executionTimeMs 执行时间（毫秒）
     * @param waitingTimeMs 等待时间（毫秒）
     */
    void updateExecutionStats(double executionTimeMs, double waitingTimeMs);

    /**
     * @brief 记录任务失败
     */
    void recordFailure() {
        totalTasksFailed++;
        lastUpdateTime_ = std::chrono::system_clock::now();
    }

    /**
     * @brief 记录任务取消
     */
    void recordCancellation() {
        totalTasksCancelled++;
        lastUpdateTime_ = std::chrono::system_clock::now();
    }

    /**
     * @brief 记录任务超时
     */
    void recordTimeout() {
        totalTasksTimeout++;
        lastUpdateTime_ = std::chrono::system_clock::now();
    }
};

}  // namespace radar
