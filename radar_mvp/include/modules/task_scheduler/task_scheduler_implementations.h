/**
 * @file task_scheduler_implementations.h
 * @brief 任务调度器实现类定义
 *
 * 定义任务调度器的具体实现类，包括队列实现和调度器实现。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#pragma once

#include "task_scheduler_interfaces.h"
#include "common/interfaces.h"
#include "common/logger.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace radar
{

    /**
     * @brief FIFO任务队列实现
     */
    class FIFOTaskQueue : public TaskQueue
    {
    public:
        FIFOTaskQueue() = default;
        ~FIFOTaskQueue() override = default;

        ErrorCode enqueue(const ScheduledTaskPtr &task) override;
        ErrorCode dequeue(ScheduledTaskPtr &task, uint32_t timeoutMs = 1000) override;
        size_t size() const override;
        bool empty() const override;
        void clear() override;

    private:
        mutable std::mutex queueMutex_;
        std::condition_variable taskAvailable_;
        std::queue<ScheduledTaskPtr> taskQueue_;
    };

    /**
     * @brief 优先级任务队列实现
     */
    class PriorityTaskQueue : public TaskQueue
    {
    public:
        PriorityTaskQueue() = default;
        ~PriorityTaskQueue() override = default;

        ErrorCode enqueue(const ScheduledTaskPtr &task) override;
        ErrorCode dequeue(ScheduledTaskPtr &task, uint32_t timeoutMs = 1000) override;
        size_t size() const override;
        bool empty() const override;
        void clear() override;

    private:
        mutable std::mutex queueMutex_;
        std::condition_variable taskAvailable_;
        std::priority_queue<ScheduledTaskPtr, std::vector<ScheduledTaskPtr>,
                            std::function<bool(const ScheduledTaskPtr &, const ScheduledTaskPtr &)>>
            taskQueue_;
    };

    /**
     * @brief 任务调度器实现类
     *
     * 实现了 ITaskScheduler 接口，提供完整的任务调度功能。
     * 支持多种调度策略和线程池管理。
     */
    class TaskScheduler : public ITaskScheduler
    {
    public:
        using TaskCompleteCallback = std::function<void(ScheduledTask::TaskId, ErrorCode)>;

    public:
        /**
         * @brief 构造函数
         * @param logger 日志记录器实例
         */
        explicit TaskScheduler(std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         */
        virtual ~TaskScheduler();

        // 禁用拷贝构造和赋值
        TaskScheduler(const TaskScheduler &) = delete;
        TaskScheduler &operator=(const TaskScheduler &) = delete;

        // 支持移动构造和赋值
        TaskScheduler(TaskScheduler &&other) noexcept;
        TaskScheduler &operator=(TaskScheduler &&other) noexcept;

        // ITaskScheduler 接口实现
        /**
         * @brief 配置任务调度参数
         * @param config 调度器配置
         * @return 操作结果错误码
         */
        ErrorCode configure(const TaskSchedulerConfig &config) override;

        /**
         * @brief 提交普通任务
         * @param task 任务函数
         * @param priority 任务优先级
         * @return 任务的future对象
         */
        std::future<void> submitTask(Task task, PacketPriority priority = PacketPriority::NORMAL) override;

        /**
         * @brief 提交有返回值的任务
         * @param task 任务函数
         * @param priority 任务优先级
         * @return 任务结果的future对象
         */
        std::future<ProcessingResultPtr> submitTaskWithResult(
            TaskWithResult task, PacketPriority priority = PacketPriority::NORMAL) override;

        /**
         * @brief 提交数据处理任务
         * @param processor 数据处理器指针
         * @param packet 数据包指针
         * @param priority 任务优先级
         * @return 处理结果的future对象
         */
        std::future<ProcessingResultPtr> submitProcessingTask(
            std::shared_ptr<IDataProcessor> processor,
            RawDataPacketPtr packet,
            PacketPriority priority = PacketPriority::NORMAL) override;

        /**
         * @brief 等待所有任务完成
         * @param timeoutMs 超时时间（毫秒），0表示无限等待
         * @return 操作结果错误码
         */
        ErrorCode waitForAllTasks(uint32_t timeoutMs = 0) override;

        /**
         * @brief 取消所有等待中的任务
         * @return 被取消的任务数量
         */
        uint32_t cancelPendingTasks() override;

        /**
         * @brief 获取调度器状态信息
         * @return 调度器状态统计
         */
        SchedulerStatus getSchedulerStatus() const override;

        // IModule 接口实现
        /**
         * @brief 初始化模块
         * @return 操作结果错误码
         */
        ErrorCode initialize() override;

        /**
         * @brief 启动模块
         * @return 操作结果错误码
         */
        ErrorCode start() override;

        /**
         * @brief 停止模块
         * @return 操作结果错误码
         */
        ErrorCode stop() override;

        /**
         * @brief 暂停模块
         * @return 操作结果错误码
         */
        ErrorCode pause() override;

        /**
         * @brief 恢复模块
         * @return 操作结果错误码
         */
        ErrorCode resume() override;

        /**
         * @brief 清理模块资源
         * @return 操作结果错误码
         */
        ErrorCode cleanup() override;

        /**
         * @brief 获取模块状态
         * @return 当前模块状态
         */
        ModuleState getState() const override;

        /**
         * @brief 获取模块名称
         * @return 模块名称字符串
         */
        const std::string &getModuleName() const override;

        /**
         * @brief 设置状态变化回调函数
         * @param callback 状态变化回调函数
         */
        void setStateChangeCallback(StateChangeCallback callback) override;

        /**
         * @brief 设置错误处理回调函数
         * @param callback 错误处理回调函数
         */
        void setErrorCallback(ErrorCallback callback) override;

        /**
         * @brief 获取模块性能统计信息
         * @return 性能统计数据的智能指针
         */
        PerformanceMetricsPtr getPerformanceMetrics() const override;

        // 扩展功能接口
        /**
         * @brief 设置调度策略
         * @param strategy 调度策略
         * @return 操作结果错误码
         */
        ErrorCode setSchedulingStrategy(SchedulingStrategy strategy);

        /**
         * @brief 获取当前调度策略
         * @return 当前调度策略
         */
        SchedulingStrategy getCurrentStrategy() const;

        /**
         * @brief 设置任务完成回调函数
         * @param callback 任务完成回调
         */
        void setTaskCompleteCallback(TaskCompleteCallback callback);

        /**
         * @brief 取消指定任务
         * @param taskId 任务ID
         * @return 操作结果错误码
         */
        ErrorCode cancelTask(ScheduledTask::TaskId taskId);

        /**
         * @brief 获取任务状态
         * @param taskId 任务ID
         * @return 任务状态
         */
        TaskState getTaskState(ScheduledTask::TaskId taskId) const;

        /**
         * @brief 获取调度统计信息
         * @param stats 输出参数，统计信息副本
         */
        void getStatistics(TaskStatistics &stats) const;

        /**
         * @brief 重置统计信息
         */
        void resetStatistics();

        /**
         * @brief 获取当前队列大小
         * @return 队列中等待的任务数量
         */
        size_t getQueueSize() const;

        /**
         * @brief 获取活跃任务数量
         * @return 正在执行的任务数量
         */
        size_t getActiveTaskCount() const;

        /**
         * @brief 设置最大并发任务数
         * @param maxConcurrent 最大并发任务数
         */
        void setMaxConcurrentTasks(uint32_t maxConcurrent);

    protected:
        /**
         * @brief 工作线程主循环
         */
        virtual void workerLoop();

        /**
         * @brief 执行任务
         * @param task 待执行任务
         * @return 操作结果错误码
         */
        virtual ErrorCode executeTask(const ScheduledTaskPtr &task);

        /**
         * @brief 验证任务对象
         * @param task 任务对象
         * @return 任务是否有效
         */
        virtual bool validateTask(const ScheduledTaskPtr &task) const;

        /**
         * @brief 任务完成回调
         * @param taskId 完成的任务ID
         * @param result 执行结果
         */
        void onTaskComplete(ScheduledTask::TaskId taskId, ErrorCode result);

        /**
         * @brief 调度错误回调
         * @param errorCode 错误代码
         * @param errorMessage 错误描述信息
         */
        void onErrorOccurred(ErrorCode errorCode, const std::string &errorMessage);

        /**
         * @brief 状态变更通知
         * @param oldState 旧状态
         * @param newState 新状态
         */
        void notifyStateChange(ModuleState oldState, ModuleState newState);

        /**
         * @brief 设置模块状态
         * @param newState 新状态
         */
        void setState(ModuleState newState);

        /**
         * @brief 验证调度器配置
         * @param config 配置参数
         * @return 配置是否有效
         */
        virtual bool validateConfig(const TaskSchedulerConfig &config) const;

        /**
         * @brief 创建任务队列
         * @param strategy 调度策略
         * @return 任务队列指针
         */
        virtual std::unique_ptr<TaskQueue> createTaskQueue(SchedulingStrategy strategy);

        /**
         * @brief 启动工作线程池
         * @param threadCount 线程数量
         * @return 操作结果错误码
         */
        ErrorCode startWorkerThreads(uint32_t threadCount);

        /**
         * @brief 停止工作线程池
         * @param timeoutMs 超时时间（毫秒）
         * @return 操作结果错误码
         */
        ErrorCode stopWorkerThreads(uint32_t timeoutMs = 5000);

        /**
         * @brief 注册活跃任务
         * @param task 任务对象
         */
        void registerActiveTask(const ScheduledTaskPtr &task);

        /**
         * @brief 注销活跃任务
         * @param taskId 任务ID
         */
        void unregisterActiveTask(ScheduledTask::TaskId taskId);

        /**
         * @brief 检查任务超时
         */
        void checkTaskTimeouts();

    protected:
        std::vector<std::thread> workerThreads_;                            ///< 工作线程池
        std::atomic<bool> running_{false};                                  ///< 运行状态标志
        std::atomic<bool> shouldStop_{false};                               ///< 停止请求标志
        std::atomic<ModuleState> currentState_{ModuleState::UNINITIALIZED}; ///< 当前模块状态

        TaskCompleteCallback taskCompleteCallback_; ///< 任务完成回调函数
        ErrorCallback errorCallback_;               ///< 错误处理回调函数
        StateChangeCallback stateChangeCallback_;   ///< 状态变化回调函数

        mutable std::mutex statsMutex_;   ///< 统计信息互斥锁
        mutable std::mutex taskMapMutex_; ///< 任务映射表互斥锁
        TaskStatistics statistics_;       ///< 调度统计信息

        std::shared_ptr<spdlog::logger> logger_;                                  ///< 日志记录器
        std::unique_ptr<TaskSchedulerConfig> config_;                             ///< 配置参数
        std::unique_ptr<TaskQueue> taskQueue_;                                    ///< 任务队列
        std::unordered_map<ScheduledTask::TaskId, ScheduledTaskPtr> activeTasks_; ///< 活跃任务映射表

        std::string moduleName_{"TaskScheduler"}; ///< 模块名称

        // 调度参数
        SchedulingStrategy currentStrategy_{SchedulingStrategy::FIFO}; ///< 当前调度策略
        std::atomic<uint32_t> maxConcurrentTasks_{4};                  ///< 最大并发任务数
        std::atomic<uint32_t> currentConcurrentTasks_{0};              ///< 当前并发任务数

        // Future管理
        mutable std::mutex futuresMutex_;                                                             ///< Future映射表互斥锁
        std::unordered_map<ScheduledTask::TaskId, std::promise<void>> promises_;                      ///< Promise映射表
        std::unordered_map<ScheduledTask::TaskId, std::promise<ProcessingResultPtr>> resultPromises_; ///< 结果Promise映射表
    };

    /**
     * @brief 线程池任务调度器
     *
     * 基于线程池的任务调度实现，支持固定线程数和动态负载均衡。
     */
    class ThreadPoolScheduler : public TaskScheduler
    {
    public:
        /**
         * @brief 构造函数
         * @param threadCount 线程池大小，0表示自动检测
         * @param logger 日志记录器实例
         */
        explicit ThreadPoolScheduler(uint32_t threadCount = 0,
                                     std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         */
        ~ThreadPoolScheduler() override = default;

    protected:
        /**
         * @brief 工作线程循环
         */
        void workerLoop() override;

    private:
        uint32_t threadCount_; ///< 线程池大小
    };

    /**
     * @brief 实时任务调度器
     *
     * 针对实时任务优化的调度器实现，支持严格的时间约束和优先级抢占。
     */
    class RealTimeScheduler : public TaskScheduler
    {
    public:
        /**
         * @brief 构造函数
         * @param logger 日志记录器实例
         */
        explicit RealTimeScheduler(std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         */
        ~RealTimeScheduler() override = default;

        /**
         * @brief 设置实时调度参数
         * @param maxLatencyMs 最大允许延迟（毫秒）
         * @param preemptionEnabled 是否启用抢占
         */
        void setRealTimeParams(uint32_t maxLatencyMs, bool preemptionEnabled);

    protected:
        /**
         * @brief 实时任务执行
         * @param task 任务对象
         * @return 操作结果错误码
         */
        ErrorCode executeTask(const ScheduledTaskPtr &task) override;

        /**
         * @brief 实时工作循环
         */
        void workerLoop() override;

    private:
        std::atomic<uint32_t> maxLatencyMs_{10};    ///< 最大延迟（毫秒）
        std::atomic<bool> preemptionEnabled_{true}; ///< 是否启用抢占

        /**
         * @brief 检查是否需要抢占
         * @param newTask 新任务
         * @param runningTask 正在执行的任务
         * @return 是否需要抢占
         */
        bool shouldPreempt(const ScheduledTaskPtr &newTask, const ScheduledTaskPtr &runningTask) const;

        /**
         * @brief 设置线程调度策略为实时
         * @return 操作结果错误码
         */
        ErrorCode setRealTimeScheduling();
    };

} // namespace radar
