/**
 * @file base_processor.cpp
 * @brief 雷达数据处理器基类实现
 *
 * 实现了数据处理器的基类DataProcessor，提供通用的处理流程、
 * 状态管理、异步任务处理等功能。采用策略模式支持运行时算法切换，
 * RAII确保资源安全管理。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @note 命名约定说明：
 *       - 成员变量：下划线后缀 (member_)
 *       - 私有方法：驼峰命名 (methodName)
 *       - 公共接口：驼峰命名 (publicMethod)
 *       - 常量：全大写下划线分隔 (MAX_BATCH_SIZE)
 *       - 原子变量：后缀说明类型 (running_, shouldStop_)
 *       - 互斥锁：后缀Mutex (statsMutex_, taskQueueMutex_)
 *       - 条件变量：描述性名称 (taskAvailable_)
 */

#include "common/error_codes.h"
#include "common/interfaces.h"
#include "common/logger.h"
#include "common/types.h"
#include "modules/data_processor.h"

// 防止Windows宏定义与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <numeric>
#include <queue>
#include <thread>

// C++17特性和STL
#include <execution>
#include <memory_resource>

namespace radar {

//==============================================================================
// 静态成员初始化
//==============================================================================

namespace {
/// 任务ID生成器
std::atomic<uint64_t> g_nextTaskId{1};

/// 默认处理超时时间（毫秒）
constexpr uint32_t DEFAULT_PROCESSING_TIMEOUT_MS = 5000;

/// 最大批处理大小
constexpr size_t MAX_BATCH_SIZE = 128;

/// 内存对齐边界（字节）
constexpr size_t MEMORY_ALIGNMENT = 32;

/**
 * @brief 生成唯一任务ID
 * @return 全局唯一的任务ID
 */
[[maybe_unused]] uint64_t generateTaskId() noexcept {
    return g_nextTaskId.fetch_add(1, std::memory_order_relaxed);
}

/**
 * @brief 检查内存对齐
 * @param ptr 内存指针
 * @param alignment 对齐边界
 * @return 是否满足对齐要求
 */
[[maybe_unused]] bool isAligned(const void *ptr, size_t alignment) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
}

/**
 * @brief 计算对齐后的大小
 * @param size 原始大小
 * @param alignment 对齐边界
 * @return 对齐后的大小
 */
[[maybe_unused]] size_t alignSize(size_t size, size_t alignment) noexcept {
    return ((size + alignment - 1) / alignment) * alignment;
}

}  // anonymous namespace

//==============================================================================
// ProcessingStatistics 实现
//==============================================================================

void ProcessingStatistics::updateStats(double processingTimeMs, size_t dataSize) {
    const uint64_t currentCount = totalPacketsProcessed.fetch_add(1) + 1;

    // 更新平均处理时间（增量计算避免精度损失）
    double currentAvg = averageProcessingTimeMs.load();
    double newAvg = currentAvg + (processingTimeMs - currentAvg) / currentCount;
    averageProcessingTimeMs.store(newAvg);

    // 更新峰值处理时间
    double currentPeak = peakProcessingTimeMs.load();
    while (processingTimeMs > currentPeak) {
        if (peakProcessingTimeMs.compare_exchange_weak(currentPeak, processingTimeMs)) {
            break;
        }
    }

    // 计算吞吐量（每秒处理的字节数）
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime_).count();

    if (duration > 1000) {  // 每秒更新一次吞吐量
        auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime_).count();
        if (totalDuration > 0) {
            // 估算总处理数据量（字节）
            double totalBytes = static_cast<double>(currentCount) * dataSize;
            throughputMbps.store(totalBytes / (totalDuration * 1024 * 1024));
        }
    }

    lastUpdateTime_ = currentTime;
}

//==============================================================================
// DataProcessor 基类实现
//==============================================================================

DataProcessor::DataProcessor(std::shared_ptr<spdlog::logger> logger)
    : logger_(logger ? logger : common::LoggerManager::getInstance().getLogger("DataProcessor")),
      currentStrategy_(ProcessingStrategy::CPU_BASIC) {
    statistics_.reset();

    MODULE_INFO(DataProcessor, "DataProcessor created with strategy: {}", static_cast<int>(currentStrategy_));
}

DataProcessor::~DataProcessor() {
    try {
        if (currentState_.load() != ModuleState::UNINITIALIZED) {
            stop();
            cleanup();
        }
    } catch (const std::exception &e) {
        // 析构函数中不应抛出异常
        if (logger_) {
            MODULE_ERROR(DataProcessor, "Exception in destructor: {}", e.what());
        }
    }
}

DataProcessor::DataProcessor(DataProcessor &&other) noexcept
    : processingThread_(std::move(other.processingThread_))  // 移动工作线程所有权
      ,
      running_(other.running_.exchange(false))  // 原子交换运行状态，源对象置为false
      ,
      shouldStop_(other.shouldStop_.exchange(false))  // 原子交换停止标志，源对象置为false
      ,
      currentState_(other.currentState_.exchange(ModuleState::UNINITIALIZED))  // 原子交换模块状态，源对象置为未初始化
      ,
      processingCallback_(std::move(other.processingCallback_))  // 移动处理完成回调函数
      ,
      errorCallback_(std::move(other.errorCallback_))  // 移动错误回调函数
      ,
      stateChangeCallback_(std::move(other.stateChangeCallback_))  // 移动状态变化回调函数
      ,
      statsMutex_()  // 互斥锁不可移动，重新构造新实例
      ,
      taskQueueMutex_()  // 任务队列互斥锁，重新构造新实例
      ,
      taskAvailable_()  // 条件变量不可移动，重新构造新实例
      ,
      taskQueue_(std::move(other.taskQueue_))  // 移动任务队列及其中的待处理任务
      ,
      statistics_()  // 处理统计信息会在构造函数体中处理
      ,
      logger_(std::move(other.logger_))  // 移动日志记录器实例
      ,
      config_(std::move(other.config_))  // 移动配置对象指针
      ,
      currentStrategy_(other.currentStrategy_)  // 复制处理策略枚举值
      ,
      moduleName_(std::move(other.moduleName_))  // 移动模块名称字符串
{
    // 移动统计信息快照到新对象，然后重置源对象统计信息
    other.statistics_.getSnapshot(statistics_);
    other.statistics_.reset();
}

DataProcessor &DataProcessor::operator=(DataProcessor &&other) noexcept {
    if (this != &other) {
        // 先清理当前对象
        try {
            if (currentState_.load() != ModuleState::UNINITIALIZED) {
                stop();
                cleanup();
            }
        } catch (...) {
            // 忽略清理异常
        }

        // 移动资源
        processingThread_ = std::move(other.processingThread_);
        running_ = other.running_.exchange(false);
        shouldStop_ = other.shouldStop_.exchange(false);
        currentState_ = other.currentState_.exchange(ModuleState::UNINITIALIZED);
        processingCallback_ = std::move(other.processingCallback_);
        errorCallback_ = std::move(other.errorCallback_);
        stateChangeCallback_ = std::move(other.stateChangeCallback_);
        taskQueue_ = std::move(other.taskQueue_);
        logger_ = std::move(other.logger_);
        config_ = std::move(other.config_);
        currentStrategy_ = other.currentStrategy_;
        moduleName_ = std::move(other.moduleName_);

        other.statistics_.getSnapshot(statistics_);
        other.statistics_.reset();
    }
    return *this;
}

//==============================================================================
// IDataProcessor 接口实现
//==============================================================================

/**
 * @brief 配置数据处理器
 * @param config 数据处理器配置参数
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 配置成功
 * @retval SystemErrors::INVALID_PARAMETER 配置参数无效或处理器状态不允许配置
 * @retval SystemErrors::CONFIGURATION_ERROR 配置过程中发生异常
 *
 * @note 只能在UNINITIALIZED状态下调用此方法
 * @warning 配置参数会影响处理器的性能和行为，请确保参数正确
 */
ErrorCode DataProcessor::configure(const DataProcessorConfig &config) {
    std::lock_guard<std::mutex> lock(statsMutex_);

    MODULE_INFO(DataProcessor, "Configuring processor with strategy: {}", static_cast<int>(config.strategy));

    // 验证配置有效性
    if (!validateConfig(config)) {
        MODULE_ERROR(DataProcessor, "Invalid configuration parameters");
        return SystemErrors::INVALID_PARAMETER;
    }

    // 只能在未初始化状态下配置
    if (currentState_.load() != ModuleState::UNINITIALIZED) {
        MODULE_ERROR(DataProcessor, "Cannot configure processor in current state: {}",
                     static_cast<int>(currentState_.load()));
        return SystemErrors::INVALID_PARAMETER;
    }

    try {
        config_ = std::make_unique<DataProcessorConfig>(config);
        currentStrategy_ = config.strategy;

        MODULE_INFO(DataProcessor, "Processor configured successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        MODULE_ERROR(DataProcessor, "Configuration failed: {}", e.what());
        return SystemErrors::CONFIGURATION_ERROR;
    }
}

ErrorCode DataProcessor::processPacket(const RawDataPacketPtr &inputPacket, ProcessingResultPtr &result) {
    // 验证处理器状态
    if (currentState_.load() != ModuleState::RUNNING) {
        MODULE_WARN(DataProcessor, "Processor not in running state");
        return DataProcessorErrors::PROCESSOR_NOT_READY;
    }

    // 验证输入数据
    if (!validateInputPacket(inputPacket)) {
        MODULE_ERROR(DataProcessor, "Invalid input packet");
        statistics_.recordFailure();
        return DataProcessorErrors::INVALID_INPUT_DATA;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 执行核心处理算法
        result = executeProcessing(inputPacket);

        if (!result) {
            MODULE_ERROR(DataProcessor, "Processing returned null result");
            statistics_.recordFailure();
            return DataProcessorErrors::PROCESSING_FAILED;
        }

        // 更新统计信息
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;

        statistics_.updateStats(duration, inputPacket->getDataSize());

        // 触发处理完成回调
        if (result->processingSuccess) {
            onProcessingComplete(*result);
        }

        MODULE_DEBUG(DataProcessor, "Packet processed successfully in {:.3f}ms", duration);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;

        MODULE_ERROR(DataProcessor, "Processing exception after {:.3f}ms: {}", duration, e.what());
        statistics_.recordFailure();
        onErrorOccurred(DataProcessorErrors::PROCESSING_FAILED, e.what());
        return DataProcessorErrors::PROCESSING_FAILED;
    }
}

/**
 * @brief 异步处理雷达数据包
 * @param inputPacket 输入的雷达数据包
 * @return std::future<ProcessingResultPtr> 异步处理结果
 * @throws ModuleException 处理器未运行或输入数据无效
 *
 * @note 此方法立即返回，实际处理在后台线程中进行
 * @note 通过返回的future可以获取处理结果或检查处理状态
 * @warning 确保在处理器停止前获取所有future的结果
 */
std::future<ProcessingResultPtr> DataProcessor::processPacketAsync(const RawDataPacketPtr &inputPacket) {
    // 验证处理器状态
    if (currentState_.load() != ModuleState::RUNNING) {
        std::promise<ProcessingResultPtr> promise;
        promise.set_exception(std::make_exception_ptr(
            ModuleException(DataProcessorErrors::PROCESSOR_NOT_READY, "Processor not in running state")));
        return promise.get_future();
    }

    // 验证输入数据
    if (!validateInputPacket(inputPacket)) {
        std::promise<ProcessingResultPtr> promise;
        promise.set_exception(
            std::make_exception_ptr(ModuleException(DataProcessorErrors::INVALID_INPUT_DATA, "Invalid input packet")));
        return promise.get_future();
    }

    // 创建promise并加入队列
    std::promise<ProcessingResultPtr> promise;
    auto future = promise.get_future();

    try {
        enqueueTask(inputPacket, std::move(promise));
    } catch (const std::exception &e) {
        std::promise<ProcessingResultPtr> errorPromise;
        errorPromise.set_exception(std::make_exception_ptr(e));
        return errorPromise.get_future();
    }

    return future;
}

/**
 * @brief 批量处理雷达数据包
 * @param inputPackets 输入的雷达数据包列表
 * @param results 输出处理结果列表
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 批量处理成功
 * @retval SystemErrors::INVALID_PARAMETER 输入参数无效（空列表或超过最大批次大小）
 * @retval DataProcessorErrors::PROCESSOR_NOT_READY 处理器未在运行状态
 * @retval DataProcessorErrors::PROCESSING_FAILED 处理过程中发生错误
 *
 * @note 批量处理可以提高吞吐量，但可能增加延迟
 * @note 结果列表的顺序与输入数据包的顺序保持一致
 */
ErrorCode DataProcessor::processBatch(const std::vector<RawDataPacketPtr> &inputPackets,
                                      std::vector<ProcessingResultPtr> &results) {
    if (inputPackets.empty()) {
        MODULE_WARN(DataProcessor, "Empty input packet batch");
        return SystemErrors::INVALID_PARAMETER;
    }

    if (inputPackets.size() > MAX_BATCH_SIZE) {
        MODULE_WARN(DataProcessor, "Batch size {} exceeds maximum {}", inputPackets.size(), MAX_BATCH_SIZE);
        return SystemErrors::INVALID_PARAMETER;
    }

    // 验证处理器状态
    if (currentState_.load() != ModuleState::RUNNING) {
        return DataProcessorErrors::PROCESSOR_NOT_READY;
    }

    results.clear();
    results.reserve(inputPackets.size());

    size_t successCount = 0;
    auto batchStartTime = std::chrono::high_resolution_clock::now();

    MODULE_DEBUG(DataProcessor, "Processing batch of {} packets", inputPackets.size());

    // 批量处理 - 可以在派生类中优化为并行处理
    for (const auto &packet : inputPackets) {
        ProcessingResultPtr result;
        ErrorCode errorCode = processPacket(packet, result);

        if (errorCode == SystemErrors::SUCCESS && result) {
            results.push_back(result);
            successCount++;
        } else {
            // 创建错误结果
            auto errorResult = std::make_shared<ProcessingResult>();
            errorResult->processingSuccess = false;
            errorResult->sourcePacketId = packet->sequenceId;
            errorResult->processingTime = std::chrono::high_resolution_clock::now();
            results.push_back(errorResult);
        }
    }

    auto batchEndTime = std::chrono::high_resolution_clock::now();
    auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchEndTime - batchStartTime).count();

    MODULE_INFO(DataProcessor, "Batch processing completed: {}/{} successful in {}ms", successCount,
                inputPackets.size(), batchDuration);

    return (successCount > 0) ? SystemErrors::SUCCESS : DataProcessorErrors::PROCESSING_FAILED;
}

/**
 * @brief 设置处理完成回调函数
 * @param callback 处理完成时的回调函数
 *
 * @note 回调函数将在每次数据包处理完成后被调用
 * @note 回调函数应当快速执行，避免阻塞处理流程
 */
void DataProcessor::setProcessingCompleteCallback(ProcessingCompleteCallback callback) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    processingCallback_ = std::move(callback);
    MODULE_DEBUG(DataProcessor, "Processing complete callback set");
}

/**
 * @brief 切换处理策略
 * @param strategy 新的处理策略
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 策略切换成功
 * @retval DataProcessorErrors::PROCESSING_FAILED 策略切换失败
 *
 * @note 在运行状态下切换策略会暂时暂停处理
 * @note 切换成功后会自动恢复处理（如果之前在运行）
 */
ErrorCode DataProcessor::switchStrategy(ProcessingStrategy strategy) {
    std::lock_guard<std::mutex> lock(statsMutex_);

    if (currentStrategy_ == strategy) {
        MODULE_DEBUG(DataProcessor, "Strategy unchanged: {}", static_cast<int>(strategy));
        return SystemErrors::SUCCESS;
    }

    MODULE_INFO(DataProcessor, "Switching strategy from {} to {}", static_cast<int>(currentStrategy_),
                static_cast<int>(strategy));

    // 在运行状态下切换策略需要特殊处理
    bool wasRunning = (currentState_.load() == ModuleState::RUNNING);
    if (wasRunning) {
        // 暂停处理
        pause();
    }

    try {
        currentStrategy_ = strategy;

        // 更新配置
        if (config_) {
            config_->strategy = strategy;
        }

        if (wasRunning) {
            // 恢复处理
            resume();
        }

        MODULE_INFO(DataProcessor, "Strategy switched successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        MODULE_ERROR(DataProcessor, "Strategy switch failed: {}", e.what());
        return DataProcessorErrors::PROCESSING_FAILED;
    }
}

/**
 * @brief 获取当前处理策略
 * @return 当前使用的处理策略
 */
ProcessingStrategy DataProcessor::getCurrentStrategy() const {
    return currentStrategy_;
}

/**
 * @brief 获取处理器能力信息
 * @return 处理器能力描述
 *
 * @note 返回的能力信息包括支持的处理策略、最大并发任务数等
 * @note 基类默认只支持CPU处理策略
 */
ProcessorCapabilities DataProcessor::getCapabilities() const {
    ProcessorCapabilities caps;
    caps.supportsCPU = true;
    caps.supportsGPU = false;  // 基类默认不支持GPU
    caps.maxConcurrentTasks = config_ ? config_->workerThreads : 4;
    caps.maxMemoryUsageMB = config_ ? config_->memoryPoolMb : 256;
    caps.supportedStrategies = {ProcessingStrategy::CPU_BASIC, ProcessingStrategy::CPU_OPTIMIZED};
    caps.processorInfo = "Base DataProcessor implementation";
    return caps;
}

//==============================================================================
// IModule 接口实现
//==============================================================================

/**
 * @brief 初始化数据处理器
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 初始化成功
 * @retval SystemErrors::ALREADY_INITIALIZED 处理器已经初始化
 * @retval SystemErrors::INITIALIZATION_ERROR 初始化过程中发生错误
 *
 * @note 必须在configure()成功调用后使用
 * @note 初始化成功后处理器状态变为INITIALIZED
 */
ErrorCode DataProcessor::initialize() {
    std::lock_guard<std::mutex> lock(statsMutex_);

    MODULE_INFO(DataProcessor, "Initializing DataProcessor");

    if (currentState_.load() != ModuleState::UNINITIALIZED) {
        MODULE_WARN(DataProcessor, "Processor already initialized");
        return SystemErrors::SUCCESS;
    }

    setState(ModuleState::INITIALIZING);

    try {
        // 验证配置
        if (!config_) {
            MODULE_ERROR(DataProcessor, "No configuration provided");
            setState(ModuleState::ERROR);
            return SystemErrors::INITIALIZATION_FAILED;
        }

        // 重置统计信息
        statistics_.reset();

        // 设置为就绪状态
        setState(ModuleState::READY);
        MODULE_INFO(DataProcessor, "DataProcessor initialized successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        MODULE_ERROR(DataProcessor, "Initialization failed: {}", e.what());
        setState(ModuleState::ERROR);
        return SystemErrors::INITIALIZATION_FAILED;
    }
}

/**
 * @brief 启动数据处理器
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 启动成功
 * @retval SystemErrors::NOT_INITIALIZED 处理器未初始化
 * @retval SystemErrors::ALREADY_RUNNING 处理器已在运行状态
 * @retval SystemErrors::THREAD_ERROR 工作线程创建失败
 *
 * @note 启动成功后处理器状态变为RUNNING
 * @note 此方法会创建后台工作线程开始处理任务
 */
ErrorCode DataProcessor::start() {
    std::lock_guard<std::mutex> lock(statsMutex_);

    MODULE_INFO(DataProcessor, "Starting DataProcessor");

    ModuleState currentState = currentState_.load();
    if (currentState != ModuleState::READY && currentState != ModuleState::PAUSED) {
        MODULE_ERROR(DataProcessor, "Cannot start from state: {}", static_cast<int>(currentState));
        return SystemErrors::INVALID_PARAMETER;
    }

    setState(ModuleState::RUNNING);

    try {
        // 启动处理线程
        shouldStop_.store(false);
        running_.store(true);

        if (!processingThread_.joinable()) {
            processingThread_ = std::thread(&DataProcessor::processingLoop, this);
        }

        MODULE_INFO(DataProcessor, "DataProcessor started successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        MODULE_ERROR(DataProcessor, "Start failed: {}", e.what());
        setState(ModuleState::ERROR);
        return SystemErrors::INITIALIZATION_FAILED;
    }
}

/**
 * @brief 停止数据处理器
 * @return 操作结果错误码
 * @retval SystemErrors::SUCCESS 停止成功
 * @retval SystemErrors::NOT_RUNNING 处理器未在运行状态
 * @retval SystemErrors::TIMEOUT 停止操作超时
 *
 * @note 此方法会安全停止工作线程并清理未完成的任务
 * @note 停止成功后处理器状态变为INITIALIZED
 * @warning 强制停止可能导致正在处理的数据丢失
 */
ErrorCode DataProcessor::stop() {
    MODULE_INFO(DataProcessor, "Stopping DataProcessor");

    // 设置停止标志
    shouldStop_.store(true);
    running_.store(false);

    // 唤醒处理线程
    taskAvailable_.notify_all();

    // 等待线程结束
    if (processingThread_.joinable()) {
        try {
            processingThread_.join();
            MODULE_DEBUG(DataProcessor, "Processing thread joined successfully");
        } catch (const std::exception &e) {
            MODULE_ERROR(DataProcessor, "Error joining processing thread: {}", e.what());
        }
    }

    setState(ModuleState::READY);
    MODULE_INFO(DataProcessor, "DataProcessor stopped successfully");
    return SystemErrors::SUCCESS;
}

ErrorCode DataProcessor::pause() {
    MODULE_INFO(DataProcessor, "Pausing DataProcessor");

    if (currentState_.load() != ModuleState::RUNNING) {
        MODULE_WARN(DataProcessor, "Processor not running, cannot pause");
        return SystemErrors::INVALID_PARAMETER;
    }

    running_.store(false);
    setState(ModuleState::PAUSED);

    MODULE_INFO(DataProcessor, "DataProcessor paused successfully");
    return SystemErrors::SUCCESS;
}

ErrorCode DataProcessor::resume() {
    MODULE_INFO(DataProcessor, "Resuming DataProcessor");

    if (currentState_.load() != ModuleState::PAUSED) {
        MODULE_WARN(DataProcessor, "Processor not paused, cannot resume");
        return SystemErrors::INVALID_PARAMETER;
    }

    running_.store(true);
    setState(ModuleState::RUNNING);
    taskAvailable_.notify_all();

    MODULE_INFO(DataProcessor, "DataProcessor resumed successfully");
    return SystemErrors::SUCCESS;
}

ErrorCode DataProcessor::cleanup() {
    MODULE_INFO(DataProcessor, "Cleaning up DataProcessor");

    // 先停止处理
    if (currentState_.load() == ModuleState::RUNNING) {
        stop();
    }

    std::lock_guard<std::mutex> lock(statsMutex_);

    try {
        // 清空任务队列
        {
            std::lock_guard<std::mutex> queueLock(taskQueueMutex_);
            while (!taskQueue_.empty()) {
                auto &task = taskQueue_.front();
                task.second.set_exception(
                    std::make_exception_ptr(ModuleException(SystemErrors::SHUTDOWN_FAILED, "System shutting down")));
                taskQueue_.pop();
            }
        }

        // 重置统计信息
        statistics_.reset();

        // 重置配置
        config_.reset();

        setState(ModuleState::UNINITIALIZED);
        MODULE_INFO(DataProcessor, "DataProcessor cleaned up successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        MODULE_ERROR(DataProcessor, "Cleanup failed: {}", e.what());
        setState(ModuleState::ERROR);
        return SystemErrors::SHUTDOWN_FAILED;
    }
}

ModuleState DataProcessor::getState() const {
    return currentState_.load();
}

const std::string &DataProcessor::getModuleName() const {
    return moduleName_;
}

void DataProcessor::setStateChangeCallback(StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stateChangeCallback_ = std::move(callback);
}

void DataProcessor::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    errorCallback_ = std::move(callback);
}

PerformanceMetricsPtr DataProcessor::getPerformanceMetrics() const {
    auto metrics = std::make_shared<SystemPerformanceMetrics>();

    // 填充处理器相关的性能指标
    metrics->dataProcessorMetrics.state = currentState_.load();
    metrics->dataProcessorMetrics.packetsProcessed = statistics_.totalPacketsProcessed.load();
    metrics->dataProcessorMetrics.packetsDropped = statistics_.processingFailures.load();
    metrics->dataProcessorMetrics.averageLatencyMs = statistics_.averageProcessingTimeMs.load();
    metrics->dataProcessorMetrics.throughputMbps = statistics_.throughputMbps.load();

    metrics->resourceUsage.cpuUsagePercent = statistics_.cpuUsagePercent.load();
    metrics->resourceUsage.memoryUsageMb = statistics_.memoryUsageBytes.load() / (1024.0 * 1024.0);
    metrics->resourceUsage.gpuUsagePercent = statistics_.gpuUsagePercent.load();

    metrics->measurementTime = std::chrono::high_resolution_clock::now();

    return metrics;
}

//==============================================================================
// 扩展功能实现
//==============================================================================

void DataProcessor::getStatistics(ProcessingStatistics &stats) const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    statistics_.getSnapshot(stats);
}

void DataProcessor::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    statistics_.reset();
    MODULE_INFO(DataProcessor, "Statistics reset");
}

//==============================================================================
// 受保护的实现方法
//==============================================================================

void DataProcessor::processingLoop() {
    MODULE_INFO(DataProcessor, "Processing loop started");

    try {
        while (!shouldStop_.load()) {
            // 检查运行状态 - 支持暂停/恢复功能
            if (!running_.load()) {
                // 暂停状态，等待唤醒信号
                // 使用条件变量避免忙等待，节省CPU资源
                std::unique_lock<std::mutex> lock(taskQueueMutex_);
                taskAvailable_.wait(lock, [this] { return shouldStop_.load() || running_.load(); });
                continue;  // 被唤醒后重新检查循环条件
            }

            // 尝试从任务队列获取待处理的数据包
            // 使用RAII确保promise和packet在异常情况下也能正确处理
            RawDataPacketPtr packet;
            std::promise<ProcessingResultPtr> promise;

            // 带超时的出队操作，避免线程永久阻塞
            // 1000ms超时确保能够定期检查停止标志
            if (!dequeueTask(packet, promise, 1000)) {
                continue;  // 超时或队列为空，继续下一次循环检查
            }

            // 执行实际的数据处理
            try {
                // 调用具体的处理算法（由子类实现）
                auto result = executeProcessing(packet);

                if (result) {
                    // 处理成功，设置promise的值供异步调用者获取
                    promise.set_value(result);

                    // 如果处理成功，触发完成回调通知上层模块
                    if (result->processingSuccess) {
                        onProcessingComplete(*result);
                    }
                } else {
                    // 处理返回空结果，设置异常供调用者处理
                    promise.set_exception(std::make_exception_ptr(
                        ModuleException(DataProcessorErrors::PROCESSING_FAILED, "Processing returned null result")));
                }
            } catch (const std::exception &e) {
                // 处理过程中发生异常，记录错误并通知调用者
                MODULE_ERROR(DataProcessor, "Processing exception: {}", e.what());
                promise.set_exception(std::make_exception_ptr(e));

                // 更新统计信息，便于监控和诊断
                statistics_.recordFailure();

                // 触发错误回调，让上层模块处理错误
                onErrorOccurred(DataProcessorErrors::PROCESSING_FAILED, e.what());
            }
        }
    } catch (const std::exception &e) {
        // 处理循环本身发生致命错误，设置错误状态
        MODULE_CRITICAL(DataProcessor, "Fatal error in processing loop: {}", e.what());
        setState(ModuleState::ERROR);
        onErrorOccurred(SystemErrors::UNKNOWN_ERROR, e.what());
    }

    MODULE_INFO(DataProcessor, "Processing loop ended");
}

bool DataProcessor::validateInputPacket(const RawDataPacketPtr &packet) const {
    if (!packet) {
        MODULE_DEBUG(DataProcessor, "Null packet pointer");
        return false;
    }

    if (!packet->isValid()) {
        MODULE_DEBUG(DataProcessor, "Invalid packet data");
        return false;
    }

    if (packet->iqData.empty()) {
        MODULE_DEBUG(DataProcessor, "Empty IQ data");
        return false;
    }

    if (packet->channelCount == 0 || packet->samplesPerChannel == 0) {
        MODULE_DEBUG(DataProcessor, "Invalid channel or sample count");
        return false;
    }

    return true;
}

void DataProcessor::onProcessingComplete(const ProcessingResult &result) {
    if (processingCallback_) {
        try {
            processingCallback_(result);
        } catch (const std::exception &e) {
            MODULE_ERROR(DataProcessor, "Processing callback exception: {}", e.what());
        }
    }
}

void DataProcessor::onErrorOccurred(ErrorCode errorCode, const std::string &errorMessage) {
    if (errorCallback_) {
        try {
            errorCallback_(errorCode, errorMessage);
        } catch (const std::exception &e) {
            MODULE_ERROR(DataProcessor, "Error callback exception: {}", e.what());
        }
    }
}

void DataProcessor::notifyStateChange(ModuleState oldState, ModuleState newState) {
    if (stateChangeCallback_) {
        try {
            stateChangeCallback_(oldState, newState);
        } catch (const std::exception &e) {
            MODULE_ERROR(DataProcessor, "State change callback exception: {}", e.what());
        }
    }
}

void DataProcessor::setState(ModuleState newState) {
    ModuleState oldState = currentState_.exchange(newState);
    if (oldState != newState) {
        MODULE_DEBUG(DataProcessor, "State changed: {} -> {}", static_cast<int>(oldState), static_cast<int>(newState));
        notifyStateChange(oldState, newState);
    }
}

bool DataProcessor::validateConfig(const DataProcessorConfig &config) const {
    if (config.workerThreads == 0 || config.workerThreads > 32) {
        MODULE_ERROR(DataProcessor, "Invalid worker thread count: {}", config.workerThreads);
        return false;
    }

    if (config.batchSize == 0 || config.batchSize > MAX_BATCH_SIZE) {
        MODULE_ERROR(DataProcessor, "Invalid batch size: {}", config.batchSize);
        return false;
    }

    if (config.processingTimeoutMs == 0 || config.processingTimeoutMs > 60000) {
        MODULE_ERROR(DataProcessor, "Invalid processing timeout: {}ms", config.processingTimeoutMs);
        return false;
    }

    if (config.memoryPoolMb == 0 || config.memoryPoolMb > 8192) {
        MODULE_ERROR(DataProcessor, "Invalid memory pool size: {}MB", config.memoryPoolMb);
        return false;
    }

    return true;
}

void DataProcessor::enqueueTask(const RawDataPacketPtr &packet, std::promise<ProcessingResultPtr> &&promise) {
    std::lock_guard<std::mutex> lock(taskQueueMutex_);

    // 检查队列大小限制，防止内存无限增长
    // 队列大小设为批次大小的4倍，提供适度的缓冲
    const size_t maxQueueSize = config_ ? (config_->batchSize * 4) : 64;
    if (taskQueue_.size() >= maxQueueSize) {
        // 队列已满，抛出异常通知调用者采取背压策略
        throw ModuleException(TaskSchedulerErrors::TASK_QUEUE_FULL, "Processing task queue is full");
    }

    // 将数据包和对应的promise作为任务加入队列
    // 使用emplace避免不必要的拷贝构造
    taskQueue_.emplace(packet, std::move(promise));

    // 通知一个等待的工作线程有新任务可处理
    taskAvailable_.notify_one();
}

bool DataProcessor::dequeueTask(RawDataPacketPtr &packet, std::promise<ProcessingResultPtr> &promise,
                                uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(taskQueueMutex_);

    // 如果队列为空，等待新任务或超时
    if (taskQueue_.empty()) {
        auto timeout = std::chrono::milliseconds(timeoutMs);
        // 条件等待：直到有任务入队或收到停止信号
        if (!taskAvailable_.wait_for(lock, timeout, [this] { return !taskQueue_.empty() || shouldStop_.load(); })) {
            return false;  // 超时返回，让调用者重新检查循环条件
        }
    }

    // 再次检查队列状态，防止竞争条件或停止信号
    if (taskQueue_.empty() || shouldStop_.load()) {
        return false;  // 队列为空或收到停止信号，返回失败
    }

    // 取出队首任务，使用move避免不必要的拷贝
    // 避免使用结构化绑定以保持C++14兼容性
    packet = std::move(taskQueue_.front().first);    // 移动数据包
    promise = std::move(taskQueue_.front().second);  // 移动promise对象

    // 移除已处理的任务，维护队列状态
    taskQueue_.pop();

    return true;  // 成功获取任务
}

}  // namespace radar
