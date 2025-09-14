/**
 * @file data_processor.h
 * @brief 雷达数据处理模块的统一接口定义和具体实现
 *
 * 定义了雷达信号处理的抽象接口以及具体实现类。
 * 支持多种处理策略：CPU基础处理、CPU优化处理、GPU加速处理。
 * 采用策略模式设计，支持运行时算法切换。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see IDataProcessor
 * @see CPUDataProcessor
 * @see GPUDataProcessor
 */

#pragma once

#include "common/interfaces.h"
#include "common/types.h"
#include "common/error_codes.h"
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
#include <chrono>
#include <future>

namespace radar
{

    /**
     * @brief 处理性能统计信息结构
     *
     * 包含数据处理过程中的各种性能指标，用于优化和监控。
     */
    struct ProcessingStatistics
    {
        std::atomic<uint64_t> totalPacketsProcessed{0};   ///< 处理的总数据包数
        std::atomic<uint64_t> processingFailures{0};      ///< 处理失败次数
        std::atomic<double> averageProcessingTimeMs{0.0}; ///< 平均处理时间（毫秒）
        std::atomic<double> peakProcessingTimeMs{0.0};    ///< 峰值处理时间（毫秒）
        std::atomic<double> throughputMbps{0.0};          ///< 处理吞吐量（Mbps）
        std::atomic<double> cpuUsagePercent{0.0};         ///< CPU使用率
        std::atomic<double> gpuUsagePercent{0.0};         ///< GPU使用率（如果适用）
        std::atomic<size_t> memoryUsageBytes{0};          ///< 内存使用量（字节）

        std::chrono::system_clock::time_point startTime_;      ///< 开始处理时间
        std::chrono::system_clock::time_point lastUpdateTime_; ///< 最后更新时间

        /**
         * @brief 重置所有统计信息
         */
        void reset()
        {
            totalPacketsProcessed = 0;
            processingFailures = 0;
            averageProcessingTimeMs = 0.0;
            peakProcessingTimeMs = 0.0;
            throughputMbps = 0.0;
            cpuUsagePercent = 0.0;
            gpuUsagePercent = 0.0;
            memoryUsageBytes = 0;
            startTime_ = std::chrono::system_clock::now();
            lastUpdateTime_ = startTime_;
        }

        /**
         * @brief 更新统计信息
         * @param processingTimeMs 当前处理时间（毫秒）
         * @param dataSize 处理的数据大小（字节）
         */
        void updateStats(double processingTimeMs, [[maybe_unused]] size_t dataSize);

        /**
         * @brief 记录处理失败
         */
        void recordFailure()
        {
            processingFailures++;
            lastUpdateTime_ = std::chrono::system_clock::now();
        }

        /**
         * @brief 获取统计信息快照
         * @param snapshot 输出参数，统计信息快照
         */
        void getSnapshot(ProcessingStatistics &snapshot) const
        {
            snapshot.totalPacketsProcessed = totalPacketsProcessed.load();
            snapshot.processingFailures = processingFailures.load();
            snapshot.averageProcessingTimeMs = averageProcessingTimeMs.load();
            snapshot.peakProcessingTimeMs = peakProcessingTimeMs.load();
            snapshot.throughputMbps = throughputMbps.load();
            snapshot.cpuUsagePercent = cpuUsagePercent.load();
            snapshot.gpuUsagePercent = gpuUsagePercent.load();
            snapshot.memoryUsageBytes = memoryUsageBytes.load();
            snapshot.startTime_ = startTime_;
            snapshot.lastUpdateTime_ = lastUpdateTime_;
        }
    };

    /**
     * @brief 抽象数据处理器基类
     *
     * 提供数据处理的通用功能实现，具体的处理算法由派生类实现。
     * 该类遵循 RAII 原则，自动管理资源和线程生命周期。
     * 实现了 IDataProcessor 接口的所有必需方法。
     *
     * @details
     * 使用流程：
     * 1. 创建具体的处理器实例
     * 2. 调用 configure() 配置处理器
     * 3. 调用 initialize() 和 start() 启动处理器
     * 4. 使用 processPacket() 或 processBatch() 处理数据
     * 5. 调用 stop() 和 cleanup() 停止处理
     * 6. 对象析构时自动清理资源
     *
     * @note 该类的所有公共方法都是线程安全的
     * @warning 在配置完成之前不要启动处理，否则可能出现未定义行为
     */
    class DataProcessor : public IDataProcessor
    {
    public:
        using ProcessingCompleteCallback = std::function<void(const ProcessingResult &)>;
        using ErrorCallback = std::function<void(ErrorCode, const std::string &)>;
        using StateChangeCallback = std::function<void(ModuleState, ModuleState)>;

    protected:
        std::thread processingThread_;                                      ///< 数据处理线程
        std::atomic<bool> running_{false};                                  ///< 运行状态标志
        std::atomic<bool> shouldStop_{false};                               ///< 停止请求标志
        std::atomic<ModuleState> currentState_{ModuleState::UNINITIALIZED}; ///< 当前模块状态

        ProcessingCompleteCallback processingCallback_; ///< 处理完成回调函数
        ErrorCallback errorCallback_;                   ///< 错误处理回调函数
        StateChangeCallback stateChangeCallback_;       ///< 状态变化回调函数

        mutable std::mutex statsMutex_;         ///< 统计信息互斥锁
        mutable std::mutex taskQueueMutex_;     ///< 任务队列互斥锁
        std::condition_variable taskAvailable_; ///< 任务可用条件变量

        std::queue<std::pair<RawDataPacketPtr, std::promise<ProcessingResultPtr>>> taskQueue_; ///< 处理任务队列
        ProcessingStatistics statistics_;                                                      ///< 处理统计信息

        std::shared_ptr<spdlog::logger> logger_;      ///< 日志记录器
        std::unique_ptr<DataProcessorConfig> config_; ///< 配置参数
        ProcessingStrategy currentStrategy_;          ///< 当前处理策略

        std::string moduleName_{"DataProcessor"}; ///< 模块名称

    public:
        /**
         * @brief 构造函数
         * @param logger 日志记录器实例
         */
        explicit DataProcessor(std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         *
         * 自动停止处理并清理所有资源。遵循 RAII 原则。
         */
        virtual ~DataProcessor();

        // 禁用拷贝构造和赋值（遵循 RAII 和单一所有权原则）
        DataProcessor(const DataProcessor &) = delete;
        DataProcessor &operator=(const DataProcessor &) = delete;

        // 支持移动构造和赋值
        DataProcessor(DataProcessor &&other) noexcept;
        DataProcessor &operator=(DataProcessor &&other) noexcept;

        // IDataProcessor 接口实现
        /**
         * @brief 配置数据处理参数
         * @param config 数据处理配置
         * @return 操作结果错误码
         */
        ErrorCode configure(const DataProcessorConfig &config) override;

        /**
         * @brief 处理单个数据包（同步方式）
         * @param inputPacket 输入数据包
         * @param result 输出参数，处理结果
         * @return 操作结果错误码
         */
        ErrorCode processPacket(const RawDataPacketPtr &inputPacket,
                                ProcessingResultPtr &result) override;

        /**
         * @brief 异步处理数据包
         * @param inputPacket 输入数据包
         * @return 处理结果的future对象
         */
        std::future<ProcessingResultPtr> processPacketAsync(
            const RawDataPacketPtr &inputPacket) override;

        /**
         * @brief 批量处理数据包
         * @param inputPackets 输入数据包列表
         * @param results 输出参数，处理结果列表
         * @return 操作结果错误码
         */
        ErrorCode processBatch(const std::vector<RawDataPacketPtr> &inputPackets,
                               std::vector<ProcessingResultPtr> &results) override;

        /**
         * @brief 设置处理完成回调函数
         * @param callback 处理完成回调
         */
        void setProcessingCompleteCallback(ProcessingCompleteCallback callback) override;

        /**
         * @brief 切换处理策略
         * @param strategy 新的处理策略
         * @return 操作结果错误码
         */
        ErrorCode switchStrategy(ProcessingStrategy strategy) override;

        /**
         * @brief 获取当前处理策略
         * @return 当前处理策略
         */
        ProcessingStrategy getCurrentStrategy() const override;

        /**
         * @brief 获取处理器能力信息
         * @return 处理器能力描述
         */
        ProcessorCapabilities getCapabilities() const override;

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
         * @brief 获取处理统计信息
         * @param stats 输出参数，统计信息副本
         * @note 该方法是线程安全的
         */
        void getStatistics(ProcessingStatistics &stats) const;

        /**
         * @brief 重置统计信息
         * @note 该方法是线程安全的
         */
        void resetStatistics();

    protected:
        /**
         * @brief 数据处理主循环（虚函数）
         *
         * 派生类可以重写该方法，提供具体的处理逻辑。
         * 该方法在独立线程中运行，直到 shouldStop_ 变为 true。
         *
         * @note 实现时需要定期检查 shouldStop_ 标志
         * @note 处理完成后应调用 onProcessingComplete() 方法
         * @note 发生错误时应调用 onErrorOccurred() 方法
         */
        virtual void processingLoop();

        /**
         * @brief 核心处理算法（纯虚函数）
         *
         * 派生类必须实现该方法，提供具体的数据处理算法。
         *
         * @param inputPacket 输入数据包
         * @return 处理结果智能指针
         */
        virtual ProcessingResultPtr executeProcessing(const RawDataPacketPtr &inputPacket) = 0;

        /**
         * @brief 验证输入数据包
         *
         * @param packet 输入数据包
         * @return 数据包是否有效
         */
        virtual bool validateInputPacket(const RawDataPacketPtr &packet) const;

        /**
         * @brief 处理完成回调
         *
         * @param result 处理结果
         */
        void onProcessingComplete(const ProcessingResult &result);

        /**
         * @brief 处理错误回调
         *
         * @param errorCode 错误代码
         * @param errorMessage 错误描述信息
         */
        void onErrorOccurred(ErrorCode errorCode, const std::string &errorMessage);

        /**
         * @brief 状态变更通知
         *
         * @param oldState 旧状态
         * @param newState 新状态
         */
        void notifyStateChange(ModuleState oldState, ModuleState newState);

        /**
         * @brief 设置模块状态
         *
         * @param newState 新状态
         */
        void setState(ModuleState newState);

        /**
         * @brief 验证处理器配置
         *
         * @param config 配置参数
         * @return 配置是否有效
         */
        virtual bool validateConfig(const DataProcessorConfig &config) const;

        /**
         * @brief 提交处理任务到队列
         *
         * @param packet 数据包
         * @param promise 结果承诺对象
         */
        void enqueueTask(const RawDataPacketPtr &packet,
                         std::promise<ProcessingResultPtr> &&promise);

        /**
         * @brief 从队列取出处理任务
         *
         * @param packet 输出参数，数据包
         * @param promise 输出参数，结果承诺对象
         * @param timeoutMs 超时时间（毫秒）
         * @return 是否成功取出任务
         */
        bool dequeueTask(RawDataPacketPtr &packet,
                         std::promise<ProcessingResultPtr> &promise,
                         uint32_t timeoutMs = 1000);
    };

    // =============================================================================
    // 具体实现类
    // =============================================================================

    /**
     * @brief CPU基础数据处理器
     *
     * 基于CPU的基础信号处理实现，适用于低延迟和小数据量场景。
     * 提供基本的FFT变换、滤波和检测算法。
     *
     * @details
     * 特性：
     * - 单线程串行处理
     * - 内存占用小
     * - 延迟低但吞吐量有限
     * - 适用于实时性要求高的场景
     *
     * @note 不支持GPU加速
     */
    class CPUDataProcessor : public DataProcessor
    {
    public:
        /**
         * @brief 构造函数
         * @param logger 日志记录器实例
         */
        explicit CPUDataProcessor(std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         */
        ~CPUDataProcessor() override = default;

        /**
         * @brief 获取处理器能力信息
         * @return CPU处理器能力描述
         */
        ProcessorCapabilities getCapabilities() const override;

    protected:
        /**
         * @brief 执行CPU处理算法
         * @param inputPacket 输入数据包
         * @return 处理结果智能指针
         */
        ProcessingResultPtr executeProcessing(const RawDataPacketPtr &inputPacket) override;

    private:
        /**
         * @brief 执行FFT变换
         * @param inputData 输入复数据
         * @param outputData 输出频域数据
         * @return 操作结果错误码
         */
        ErrorCode performFFT(const AlignedComplexVector &inputData,
                             AlignedComplexVector &outputData);

        /**
         * @brief 执行数字滤波
         * @param inputData 输入数据
         * @param outputData 输出数据
         * @return 操作结果错误码
         */
        ErrorCode performFiltering(const AlignedComplexVector &inputData,
                                   AlignedComplexVector &outputData);

        /**
         * @brief 执行目标检测
         * @param inputData 输入数据
         * @param detectionResults 检测结果
         * @return 操作结果错误码
         */
        ErrorCode performDetection(const AlignedComplexVector &inputData,
                                   std::vector<double> &detectionResults);

        /**
         * @brief 执行波束形成
         * @param inputData 输入多通道数据
         * @param beamformedData 波束形成结果
         * @return 操作结果错误码
         */
        ErrorCode performBeamforming(const std::vector<AlignedComplexVector> &inputData,
                                     AlignedComplexVector &beamformedData);

        /**
         * @brief 获取当前CPU使用率
         * @return CPU使用率百分比
         */
        double getCurrentCPUUsage() const;

        /**
         * @brief 估算处理过程的内存使用量
         * @param packet 输入数据包
         * @return 估计的内存使用量（字节）
         */
        size_t estimateMemoryUsage(const RawDataPacketPtr &packet) const;
    };

    /**
     * @brief GPU加速数据处理器
     *
     * 基于GPU CUDA的高性能信号处理实现，适用于大数据量和高吞吐量场景。
     * 支持并行FFT、并行滤波和GPU内存优化。
     *
     * @details
     * 特性：
     * - CUDA并行计算加速
     * - 高吞吐量处理能力
     * - GPU内存池管理
     * - 支持多流并发处理
     *
     * @note 需要CUDA兼容的GPU设备
     * @warning 需要在编译时启用CUDA支持
     */
    class GPUDataProcessor : public DataProcessor
    {
    public:
        /**
         * @brief 构造函数
         * @param logger 日志记录器实例
         */
        explicit GPUDataProcessor(std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 析构函数
         */
        ~GPUDataProcessor() override;

        /**
         * @brief 获取处理器能力信息
         * @return GPU处理器能力描述
         */
        ProcessorCapabilities getCapabilities() const override;

        /**
         * @brief 初始化GPU资源
         * @return 操作结果错误码
         */
        ErrorCode initialize() override;

        /**
         * @brief 清理GPU资源
         * @return 操作结果错误码
         */
        ErrorCode cleanup() override;

    protected:
        /**
         * @brief 执行GPU处理算法
         * @param inputPacket 输入数据包
         * @return 处理结果智能指针
         */
        ProcessingResultPtr executeProcessing(const RawDataPacketPtr &inputPacket) override;

    private:
        void *gpuContext_;        ///< GPU上下文指针（具体类型依赖CUDA实现）
        void *deviceMemory_;      ///< GPU设备内存指针
        size_t deviceMemorySize_; ///< GPU设备内存大小
        int deviceId_;            ///< GPU设备ID

        /**
         * @brief 初始化GPU设备和上下文
         * @return 操作结果错误码
         */
        ErrorCode initializeGPU();

        /**
         * @brief 清理GPU资源
         * @return 操作结果错误码
         */
        ErrorCode cleanupGPU();

        /**
         * @brief 将数据传输到GPU
         * @param hostData 主机端数据
         * @param devicePtr GPU设备指针
         * @param size 数据大小
         * @return 操作结果错误码
         */
        ErrorCode copyToDevice(const void *hostData, void *devicePtr, size_t size);

        /**
         * @brief 从GPU传输数据到主机
         * @param devicePtr GPU设备指针
         * @param hostData 主机端数据
         * @param size 数据大小
         * @return 操作结果错误码
         */
        ErrorCode copyFromDevice(const void *devicePtr, void *hostData, size_t size);

        /**
         * @brief 执行GPU FFT计算
         * @param inputData GPU输入数据指针
         * @param outputData GPU输出数据指针
         * @param size 数据大小
         * @return 操作结果错误码
         */
        ErrorCode performGPUFFT(void *inputData, void *outputData, size_t size);

        /**
         * @brief 检查GPU设备能力
         * @return GPU是否可用且兼容
         */
        bool checkGPUCapabilities() const;

        /**
         * @brief 在GPU上处理数据包
         * @param inputPacket 输入数据包
         * @return 处理结果智能指针
         */
        ProcessingResultPtr processOnGPU(const RawDataPacketPtr &inputPacket);

        /**
         * @brief 在CPU上处理数据包（回退方案）
         * @param inputPacket 输入数据包
         * @return 处理结果智能指针
         */
        ProcessingResultPtr processByCPU(const RawDataPacketPtr &inputPacket);

        /**
         * @brief 获取当前GPU使用率
         * @return GPU使用率百分比
         */
        double getCurrentGPUUsage() const;

        /**
         * @brief 获取当前CPU使用率
         * @return CPU使用率百分比
         */
        double getCurrentCPUUsage() const;

        /**
         * @brief 估算处理过程的内存使用量
         * @param packet 输入数据包
         * @return 估计的内存使用量（字节）
         */
        size_t estimateMemoryUsage(const RawDataPacketPtr &packet) const;
    };

    // =============================================================================
    // 工厂函数
    // =============================================================================

    /**
     * @brief 数据处理器工厂命名空间
     */
    namespace DataProcessorFactory
    {
        /**
         * @brief 数据处理器类型枚举
         */
        enum class ProcessorType
        {
            CPU_PROCESSOR,   ///< CPU处理器
            GPU_PROCESSOR,   ///< GPU处理器
            HYBRID_PROCESSOR ///< 混合处理器（预留）
        };

        /**
         * @brief 创建CPU数据处理器
         *
         * @param config 处理器配置
         * @param logger 日志记录器实例
         * @return CPU处理器智能指针，创建失败时返回 nullptr
         */
        std::unique_ptr<CPUDataProcessor> createCPUProcessor(
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 创建GPU数据处理器
         *
         * @param config 处理器配置
         * @param logger 日志记录器实例
         * @return GPU处理器智能指针，创建失败时返回 nullptr
         */
        std::unique_ptr<GPUDataProcessor> createGPUProcessor(
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 自动创建合适的数据处理器
         *
         * 根据配置参数和系统能力自动选择最合适的处理器类型。
         *
         * @param processorType 处理器类型
         * @param config 处理器配置
         * @param logger 日志记录器实例
         * @return 数据处理器智能指针，创建失败时返回 nullptr
         */
        std::unique_ptr<DataProcessor> createProcessor(
            ProcessorType processorType,
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger = nullptr);

        /**
         * @brief 检查处理器类型是否可用
         *
         * @param processorType 处理器类型
         * @return 处理器类型是否可用
         */
        bool isProcessorTypeAvailable(ProcessorType processorType);

    } // namespace DataProcessorFactory

} // namespace radar
