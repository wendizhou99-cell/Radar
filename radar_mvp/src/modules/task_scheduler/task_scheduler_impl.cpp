/**
 * @file task_scheduler_impl.cpp
 * @brief 任务调度器核心实现
 *
 * 实现任务调度器的核心功能和算法。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include <algorithm>
#include <chrono>
#include <thread>

#include "common/error_codes.h"
#include "common/logger.h"
#include "modules/task_scheduler/task_scheduler_implementations.h"

namespace radar {

// TaskScheduler 实现
TaskScheduler::TaskScheduler(std::shared_ptr<spdlog::logger> logger) : logger_(logger) {
    if (!logger_) {
        // 使用默认日志器
        logger_ = spdlog::default_logger();
    }

    RADAR_INFO("TaskScheduler created");
    setState(ModuleState::UNINITIALIZED);
}

TaskScheduler::~TaskScheduler() {
    if (running_.load()) {
        stop();
    }
    RADAR_INFO("TaskScheduler destroyed");
}

TaskScheduler::TaskScheduler(TaskScheduler &&other) noexcept
    : running_(other.running_.load()),
      shouldStop_(other.shouldStop_.load()),
      currentState_(other.currentState_.load()),
      taskCompleteCallback_(std::move(other.taskCompleteCallback_)),
      errorCallback_(std::move(other.errorCallback_)),
      stateChangeCallback_(std::move(other.stateChangeCallback_)),
      statistics_(other.statistics_),  // 现在可以使用复制构造
      logger_(std::move(other.logger_)),
      config_(std::move(other.config_)),
      taskQueue_(std::move(other.taskQueue_)),
      activeTasks_(std::move(other.activeTasks_)),
      moduleName_(std::move(other.moduleName_)),
      currentStrategy_(other.currentStrategy_),
      maxConcurrentTasks_(other.maxConcurrentTasks_.load()),
      currentConcurrentTasks_(other.currentConcurrentTasks_.load()),
      promises_(std::move(other.promises_)),
      resultPromises_(std::move(other.resultPromises_)) {
    // 移动后重置原对象状态
    other.running_ = false;
    other.shouldStop_ = false;
    other.currentState_ = ModuleState::UNINITIALIZED;
    other.currentConcurrentTasks_ = 0;
}

TaskScheduler &TaskScheduler::operator=(TaskScheduler &&other) noexcept {
    if (this != &other) {
        // 先停止当前实例
        if (running_.load()) {
            stop();
        }

        running_ = other.running_.load();
        shouldStop_ = other.shouldStop_.load();
        currentState_ = other.currentState_.load();
        taskCompleteCallback_ = std::move(other.taskCompleteCallback_);
        errorCallback_ = std::move(other.errorCallback_);
        stateChangeCallback_ = std::move(other.stateChangeCallback_);
        logger_ = std::move(other.logger_);
        config_ = std::move(other.config_);
        taskQueue_ = std::move(other.taskQueue_);
        activeTasks_ = std::move(other.activeTasks_);
        moduleName_ = std::move(other.moduleName_);
        currentStrategy_ = other.currentStrategy_;
        maxConcurrentTasks_ = other.maxConcurrentTasks_.load();
        currentConcurrentTasks_ = other.currentConcurrentTasks_.load();
        promises_ = std::move(other.promises_);
        resultPromises_ = std::move(other.resultPromises_);

        // 复制统计信息
        statistics_.totalTasksSubmitted.store(other.statistics_.totalTasksSubmitted.load());
        statistics_.totalTasksCompleted.store(other.statistics_.totalTasksCompleted.load());
        statistics_.totalTasksFailed.store(other.statistics_.totalTasksFailed.load());
        statistics_.totalTasksCancelled.store(other.statistics_.totalTasksCancelled.load());
        statistics_.totalTasksTimeout.store(other.statistics_.totalTasksTimeout.load());
        statistics_.currentPendingTasks.store(other.statistics_.currentPendingTasks.load());
        statistics_.currentRunningTasks.store(other.statistics_.currentRunningTasks.load());
        statistics_.averageExecutionTimeMs.store(other.statistics_.averageExecutionTimeMs.load());
        statistics_.averageWaitingTimeMs.store(other.statistics_.averageWaitingTimeMs.load());
        statistics_.throughputTasksPerSecond.store(other.statistics_.throughputTasksPerSecond.load());
        statistics_.startTime_ = other.statistics_.startTime_;
        statistics_.lastUpdateTime_ = other.statistics_.lastUpdateTime_;

        // 重置原对象
        other.running_ = false;
        other.shouldStop_ = false;
        other.currentState_ = ModuleState::UNINITIALIZED;
        other.currentConcurrentTasks_ = 0;
    }
    return *this;
}

ErrorCode TaskScheduler::configure(const TaskSchedulerConfig &config) {
    if (!validateConfig(config)) {
        RADAR_ERROR("Invalid scheduler configuration");
        return SystemErrors::INVALID_PARAMETER;
    }

    config_ = std::make_unique<TaskSchedulerConfig>(config);
    // 从schedulingPolicy字符串映射到枚举
    if (config.schedulingPolicy == "fifo") {
        currentStrategy_ = SchedulingStrategy::FIFO;
    } else if (config.schedulingPolicy == "priority") {
        currentStrategy_ = SchedulingStrategy::PRIORITY;
    } else {
        currentStrategy_ = SchedulingStrategy::FIFO;  // 默认
    }
    maxConcurrentTasks_ = config.maxThreads;  // 使用maxThreads作为并发任务数

    // 创建任务队列
    taskQueue_ = createTaskQueue(currentStrategy_);

    RADAR_INFO("TaskScheduler configured with strategy {} and {} max concurrent tasks", config.schedulingPolicy,
               maxConcurrentTasks_.load());
    return SystemErrors::SUCCESS;
}

std::future<void> TaskScheduler::submitTask(Task task, PacketPriority priority) {
    if (!task) {
        RADAR_ERROR("Cannot submit null task");
        throw std::invalid_argument("Task cannot be null");
    }

    auto scheduledTask = std::make_shared<ScheduledTask>(std::move(task), priority, 0, "");

    std::promise<void> promise;
    auto future = promise.get_future();

    {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        promises_[scheduledTask->getId()] = std::move(promise);
    }

    ErrorCode result = taskQueue_->enqueue(scheduledTask);
    if (result != SystemErrors::SUCCESS) {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        promises_.erase(scheduledTask->getId());
        RADAR_ERROR("Failed to enqueue task: {}", static_cast<int>(result));
        throw std::runtime_error("Failed to enqueue task");
    }

    statistics_.totalTasksSubmitted++;
    statistics_.currentPendingTasks++;

    RADAR_DEBUG("Submitted task {} with priority {}", scheduledTask->getId(), static_cast<int>(priority));
    return future;
}

std::future<ProcessingResultPtr> TaskScheduler::submitTaskWithResult(TaskWithResult task, PacketPriority priority) {
    if (!task) {
        RADAR_ERROR("Cannot submit null task with result");
        throw std::invalid_argument("Task cannot be null");
    }

    auto scheduledTask = std::make_shared<ScheduledTask>(
        [task]() {
            // 这里需要包装任务以返回结果
            // 简化实现，实际需要更复杂的处理
        },
        priority, 0, "");

    std::promise<ProcessingResultPtr> promise;
    auto future = promise.get_future();

    {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        resultPromises_[scheduledTask->getId()] = std::move(promise);
    }

    ErrorCode result = taskQueue_->enqueue(scheduledTask);
    if (result != SystemErrors::SUCCESS) {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        resultPromises_.erase(scheduledTask->getId());
        RADAR_ERROR("Failed to enqueue task with result: {}", static_cast<int>(result));
        throw std::runtime_error("Failed to enqueue task");
    }

    statistics_.totalTasksSubmitted++;
    statistics_.currentPendingTasks++;

    RADAR_DEBUG("Submitted task with result {} with priority {}", scheduledTask->getId(), static_cast<int>(priority));
    return future;
}

std::future<ProcessingResultPtr> TaskScheduler::submitProcessingTask(std::shared_ptr<IDataProcessor> processor,
                                                                     RawDataPacketPtr packet, PacketPriority priority) {
    if (!processor || !packet) {
        RADAR_ERROR("Invalid processor or packet for processing task");
        throw std::invalid_argument("Processor and packet cannot be null");
    }

    auto task = [processor, packet]() -> ProcessingResultPtr {
        ProcessingResultPtr result;
        ErrorCode status = processor->processPacket(packet, result);
        if (status != SystemErrors::SUCCESS) {
            // 创建一个表示错误的结果
            result = std::make_shared<ProcessingResult>();
            // 设置错误状态等
        }
        return result;
    };

    return submitTaskWithResult(task, priority);
}

ErrorCode TaskScheduler::waitForAllTasks(uint32_t timeoutMs) {
    if (!running_.load()) {
        RADAR_WARN("Scheduler is not running");
        return TaskSchedulerErrors::SCHEDULER_NOT_READY;
    }

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = timeoutMs == 0 ? std::chrono::milliseconds::max() : std::chrono::milliseconds(timeoutMs);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(statsMutex_);
            if (statistics_.currentPendingTasks.load() == 0 && statistics_.currentRunningTasks.load() == 0) {
                break;
            }
        }

        if (std::chrono::steady_clock::now() - startTime > timeout) {
            RADAR_WARN("Timeout waiting for all tasks to complete");
            return SystemErrors::OPERATION_TIMEOUT;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    RADAR_INFO("All tasks completed");
    return SystemErrors::SUCCESS;
}

uint32_t TaskScheduler::cancelPendingTasks() {
    uint32_t cancelledCount = 0;

    // 这里需要实现取消逻辑
    // 简化实现，实际需要遍历队列并取消任务

    RADAR_INFO("Cancelled {} pending tasks", cancelledCount);
    return cancelledCount;
}

SchedulerStatus TaskScheduler::getSchedulerStatus() const {
    SchedulerStatus status;
    status.activeThreads = currentConcurrentTasks_.load();
    status.pendingTasks = statistics_.currentPendingTasks.load();
    status.completedTasks = statistics_.totalTasksCompleted.load();
    status.failedTasks = statistics_.totalTasksFailed.load();
    status.averageExecutionTimeMs = statistics_.averageExecutionTimeMs.load();
    status.throughputTasksPerSec = statistics_.throughputTasksPerSecond.load();
    status.schedulerState = getState();
    return status;
}

// IModule 接口实现
ErrorCode TaskScheduler::initialize() {
    if (getState() != ModuleState::UNINITIALIZED) {
        RADAR_WARN("Scheduler already initialized");
        return SystemErrors::SUCCESS;
    }

    if (!config_) {
        RADAR_ERROR("Scheduler not configured");
        return SystemErrors::CONFIGURATION_ERROR;
    }

    statistics_.reset();
    setState(ModuleState::READY);

    RADAR_INFO("TaskScheduler initialized");
    return SystemErrors::SUCCESS;
}

ErrorCode TaskScheduler::start() {
    if (getState() != ModuleState::READY) {
        RADAR_ERROR("Scheduler not ready to start");
        return TaskSchedulerErrors::SCHEDULER_NOT_READY;
    }

    if (running_.load()) {
        RADAR_WARN("Scheduler already running");
        return SystemErrors::SUCCESS;
    }

    running_ = true;
    shouldStop_ = false;

    ErrorCode result = startWorkerThreads(config_->coreThreads);
    if (result != SystemErrors::SUCCESS) {
        running_ = false;
        RADAR_ERROR("Failed to start worker threads");
        return result;
    }

    setState(ModuleState::RUNNING);
    RADAR_INFO("TaskScheduler started with {} threads", config_->coreThreads);
    return SystemErrors::SUCCESS;
}

ErrorCode TaskScheduler::stop() {
    if (!running_.load()) {
        RADAR_WARN("Scheduler not running");
        return SystemErrors::SUCCESS;
    }

    shouldStop_ = true;
    running_ = false;

    ErrorCode result = stopWorkerThreads();
    if (result != SystemErrors::SUCCESS) {
        RADAR_ERROR("Failed to stop worker threads gracefully");
    }

    setState(ModuleState::READY);
    RADAR_INFO("TaskScheduler stopped");
    return SystemErrors::SUCCESS;
}

ErrorCode TaskScheduler::pause() {
    if (getState() != ModuleState::RUNNING) {
        RADAR_ERROR("Cannot pause scheduler in state {}", static_cast<int>(getState()));
        return TaskSchedulerErrors::SCHEDULER_NOT_READY;
    }

    // 暂停逻辑实现
    setState(ModuleState::PAUSED);
    RADAR_INFO("TaskScheduler paused");
    return SystemErrors::SUCCESS;
}

ErrorCode TaskScheduler::resume() {
    if (getState() != ModuleState::PAUSED) {
        RADAR_ERROR("Cannot resume scheduler in state {}", static_cast<int>(getState()));
        return TaskSchedulerErrors::SCHEDULER_NOT_READY;
    }

    // 恢复逻辑实现
    setState(ModuleState::RUNNING);
    RADAR_INFO("TaskScheduler resumed");
    return SystemErrors::SUCCESS;
}

ErrorCode TaskScheduler::cleanup() {
    if (running_.load()) {
        stop();
    }

    // 清理资源
    {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        promises_.clear();
        resultPromises_.clear();
    }

    {
        std::unique_lock<std::mutex> lock(taskMapMutex_);
        activeTasks_.clear();
    }

    if (taskQueue_) {
        taskQueue_->clear();
    }

    statistics_.reset();
    setState(ModuleState::UNINITIALIZED);

    RADAR_INFO("TaskScheduler cleaned up");
    return SystemErrors::SUCCESS;
}

ModuleState TaskScheduler::getState() const {
    return currentState_.load();
}

const std::string &TaskScheduler::getModuleName() const {
    return moduleName_;
}

void TaskScheduler::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback_ = std::move(callback);
}

void TaskScheduler::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

PerformanceMetricsPtr TaskScheduler::getPerformanceMetrics() const {
    auto metrics = std::make_shared<SystemPerformanceMetrics>();
    // 填充性能指标
    metrics->resourceUsage.cpuUsagePercent = 0.0;  // 简化实现
    metrics->resourceUsage.memoryUsageMb = 0.0;
    metrics->resourceUsage.gpuUsagePercent = 0.0;
    metrics->resourceUsage.gpuMemoryUsageMb = 0.0;
    return metrics;
}

// 扩展功能实现
ErrorCode TaskScheduler::setSchedulingStrategy(SchedulingStrategy strategy) {
    if (running_.load()) {
        RADAR_ERROR("Cannot change strategy while running");
        return TaskSchedulerErrors::SCHEDULER_NOT_READY;
    }

    currentStrategy_ = strategy;
    taskQueue_ = createTaskQueue(strategy);

    RADAR_INFO("Scheduling strategy changed to {}", static_cast<int>(strategy));
    return SystemErrors::SUCCESS;
}

SchedulingStrategy TaskScheduler::getCurrentStrategy() const {
    return currentStrategy_;
}

void TaskScheduler::setTaskCompleteCallback(TaskCompleteCallback callback) {
    taskCompleteCallback_ = std::move(callback);
}

ErrorCode TaskScheduler::cancelTask(ScheduledTask::TaskId taskId) {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    auto it = activeTasks_.find(taskId);
    if (it != activeTasks_.end()) {
        return it->second->cancel();
    }

    RADAR_WARN("Task {} not found for cancellation", taskId);
    return TaskSchedulerErrors::SCHEDULING_ERROR;
}

TaskState TaskScheduler::getTaskState(ScheduledTask::TaskId taskId) const {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    auto it = activeTasks_.find(taskId);
    if (it != activeTasks_.end()) {
        return it->second->getState();
    }
    return TaskState::PENDING;  // 默认状态
}

void TaskScheduler::getStatistics(TaskStatistics &stats) const {
    std::unique_lock<std::mutex> lock(statsMutex_);
    stats = statistics_;
}

void TaskScheduler::resetStatistics() {
    std::unique_lock<std::mutex> lock(statsMutex_);
    statistics_.reset();
    RADAR_INFO("TaskScheduler statistics reset");
}

size_t TaskScheduler::getQueueSize() const {
    return taskQueue_ ? taskQueue_->size() : 0;
}

size_t TaskScheduler::getActiveTaskCount() const {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    return activeTasks_.size();
}

void TaskScheduler::setMaxConcurrentTasks(uint32_t maxConcurrent) {
    maxConcurrentTasks_ = maxConcurrent;
    RADAR_INFO("Max concurrent tasks set to {}", maxConcurrent);
}

// 保护方法实现
void TaskScheduler::workerLoop() {
    RADAR_DEBUG("Worker thread started");

    while (!shouldStop_.load()) {
        ScheduledTaskPtr task;
        ErrorCode result = taskQueue_->dequeue(task, 100);  // 100ms 超时

        if (result == SystemErrors::SUCCESS && task) {
            if (currentConcurrentTasks_.load() < maxConcurrentTasks_.load()) {
                currentConcurrentTasks_++;
                statistics_.currentPendingTasks--;

                // 异步执行任务
                std::thread([this, task]() {
                    executeTask(task);
                    currentConcurrentTasks_--;
                }).detach();
            } else {
                // 重新入队
                taskQueue_->enqueue(task);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else if (result == TaskSchedulerErrors::TASK_TIMEOUT) {
            // 超时，继续循环
            continue;
        } else {
            // 其他错误，记录并继续
            RADAR_ERROR("Failed to dequeue task: {}", static_cast<int>(result));
        }
    }

    RADAR_DEBUG("Worker thread stopped");
}

ErrorCode TaskScheduler::executeTask(const ScheduledTaskPtr &task) {
    if (!task) {
        return TaskSchedulerErrors::TASK_EXECUTION_FAILED;
    }

    registerActiveTask(task);
    statistics_.currentRunningTasks++;

    auto startTime = std::chrono::steady_clock::now();
    ErrorCode result = task->execute();
    auto endTime = std::chrono::steady_clock::now();

    statistics_.currentRunningTasks--;

    double executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    if (result == SystemErrors::SUCCESS) {
        statistics_.totalTasksCompleted++;
        statistics_.updateExecutionStats(executionTime, task->getWaitingTimeMs());
    } else {
        statistics_.totalTasksFailed++;
        statistics_.recordFailure();
    }

    unregisterActiveTask(task->getId());
    onTaskComplete(task->getId(), result);

    return result;
}

bool TaskScheduler::validateTask(const ScheduledTaskPtr &task) const {
    return task && task->getId() > 0;
}

void TaskScheduler::onTaskComplete(ScheduledTask::TaskId taskId, ErrorCode result) {
    // 设置 promise
    {
        std::unique_lock<std::mutex> lock(futuresMutex_);
        auto it = promises_.find(taskId);
        if (it != promises_.end()) {
            if (result == SystemErrors::SUCCESS) {
                it->second.set_value();
            } else {
                it->second.set_exception(std::make_exception_ptr(std::runtime_error("Task execution failed")));
            }
            promises_.erase(it);
        }

        auto rit = resultPromises_.find(taskId);
        if (rit != resultPromises_.end()) {
            if (result == SystemErrors::SUCCESS) {
                // 这里需要实际的处理结果
                rit->second.set_value(nullptr);
            } else {
                rit->second.set_exception(std::make_exception_ptr(std::runtime_error("Task execution failed")));
            }
            resultPromises_.erase(rit);
        }
    }

    // 调用回调
    if (taskCompleteCallback_) {
        taskCompleteCallback_(taskId, result);
    }
}

void TaskScheduler::onErrorOccurred(ErrorCode errorCode, const std::string &errorMessage) {
    RADAR_ERROR("Scheduler error {}: {}", static_cast<int>(errorCode), errorMessage);

    if (errorCallback_) {
        errorCallback_(errorCode, errorMessage);
    }
}

void TaskScheduler::notifyStateChange(ModuleState oldState, ModuleState newState) {
    if (stateChangeCallback_) {
        stateChangeCallback_(oldState, newState);
    }
}

void TaskScheduler::setState(ModuleState newState) {
    ModuleState oldState = currentState_.load();
    if (oldState != newState) {
        currentState_.store(newState);
        notifyStateChange(oldState, newState);
    }
}

bool TaskScheduler::validateConfig(const TaskSchedulerConfig &config) const {
    return config.coreThreads > 0 && config.maxThreads > 0 && config.queueCapacity > 0;
}

std::unique_ptr<TaskQueue> TaskScheduler::createTaskQueue(SchedulingStrategy strategy) {
    switch (strategy) {
        case SchedulingStrategy::FIFO:
            return std::make_unique<FIFOTaskQueue>();
        case SchedulingStrategy::PRIORITY:
            return std::make_unique<PriorityTaskQueue>();
        default:
            RADAR_WARN("Unknown scheduling strategy {}, using FIFO", static_cast<int>(strategy));
            return std::make_unique<FIFOTaskQueue>();
    }
}

ErrorCode TaskScheduler::startWorkerThreads(uint32_t threadCount) {
    try {
        workerThreads_.reserve(threadCount);
        for (uint32_t i = 0; i < threadCount; ++i) {
            workerThreads_.emplace_back([this]() { workerLoop(); });
        }
        RADAR_INFO("Started {} worker threads", threadCount);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to start worker threads: {}", e.what());
        return TaskSchedulerErrors::THREAD_POOL_ERROR;
    }
}

ErrorCode TaskScheduler::stopWorkerThreads(uint32_t timeoutMs) {
    shouldStop_ = true;

    // 等待线程结束
    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeoutMs);

    for (auto &thread : workerThreads_) {
        if (thread.joinable()) {
            if (std::chrono::steady_clock::now() - startTime > timeout) {
                RADAR_WARN("Timeout waiting for worker threads to stop");
                return TaskSchedulerErrors::TASK_TIMEOUT;
            }
            thread.join();
        }
    }

    workerThreads_.clear();
    RADAR_INFO("All worker threads stopped");
    return SystemErrors::SUCCESS;
}

void TaskScheduler::registerActiveTask(const ScheduledTaskPtr &task) {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    activeTasks_[task->getId()] = task;
}

void TaskScheduler::unregisterActiveTask(ScheduledTask::TaskId taskId) {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    activeTasks_.erase(taskId);
}

void TaskScheduler::checkTaskTimeouts() {
    std::unique_lock<std::mutex> lock(taskMapMutex_);
    for (auto it = activeTasks_.begin(); it != activeTasks_.end();) {
        if (it->second->isTimeout()) {
            it->second->cancel();
            statistics_.recordTimeout();
            it = activeTasks_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace radar
