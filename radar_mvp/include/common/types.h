/**
 * @file types.h
 * @brief 雷达MVP系统核心数据类型定义
 *
 * 定义了整个雷达数据处理系统中使用的核心数据结构，包括数据包格式、
 * 处理结果、配置参数和状态信息等。所有数据类型都针对GPU处理进行了优化。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see interfaces.h
 * @see error_codes.h
 */

#pragma once

#include <chrono>
#include <complex>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace radar {

//==============================================================================
// 基础类型别名
//==============================================================================

/// 时间戳类型（微秒精度）
using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

/// 复数数据类型（I/Q信号）
using ComplexFloat = std::complex<float>;
using ComplexDouble = std::complex<double>;

/// GPU内存对齐的浮点数组
using AlignedFloatVector = std::vector<float>;
using AlignedComplexVector = std::vector<ComplexFloat>;

//==============================================================================
// 系统状态枚举
//==============================================================================

/**
 * @brief 模块运行状态枚举
 * @details 定义了所有模块通用的状态机状态
 */
enum class ModuleState : uint8_t {
    UNINITIALIZED = 0,  ///< 未初始化状态
    INITIALIZING,       ///< 正在初始化
    READY,              ///< 就绪状态，可以接收任务
    RUNNING,            ///< 运行状态，正在处理数据
    PAUSED,             ///< 暂停状态
    ERROR,              ///< 错误状态
    SHUTDOWN            ///< 关闭状态
};

/**
 * @brief 数据处理策略枚举
 * @details 定义了不同的数据处理实现策略
 */
enum class ProcessingStrategy : uint8_t {
    CPU_BASIC = 0,    ///< 基础CPU处理策略
    CPU_OPTIMIZED,    ///< 优化的CPU处理策略
    GPU_ACCELERATED,  ///< GPU加速处理策略
    HYBRID            ///< 混合处理策略
};

/**
 * @brief 数据包优先级枚举
 * @details 用于任务调度的优先级控制
 */
enum class PacketPriority : uint8_t {
    LOW = 0,  ///< 低优先级
    NORMAL,   ///< 普通优先级
    HIGH,     ///< 高优先级
    CRITICAL  ///< 关键优先级
};

//==============================================================================
// 配置参数结构体
//==============================================================================

/**
 * @brief 数据接收配置参数
 * @details 控制数据接收模块的行为参数
 */
struct DataReceiverConfig {
    bool simulationEnabled = true;               ///< 是否启用模拟数据源
    uint32_t dataRateMbps = 100;                 ///< 数据传输速率(Mbps)
    uint32_t packetSizeBytes = 4096;             ///< 数据包大小(字节)
    uint32_t generationIntervalMs = 10;          ///< 数据生成间隔(毫秒)
    uint32_t maxQueueSize = 1000;                ///< 最大队列大小
    std::string overflowPolicy = "drop_oldest";  ///< 溢出处理策略
};

/**
 * @brief 数据处理配置参数
 * @details 控制数据处理模块的算法和性能参数
 */
struct DataProcessorConfig {
    ProcessingStrategy strategy = ProcessingStrategy::CPU_BASIC;  ///< 处理策略
    uint32_t workerThreads = 4;                                   ///< 工作线程数量
    uint32_t batchSize = 16;                                      ///< 批处理大小
    uint32_t processingTimeoutMs = 100;                           ///< 处理超时时间(毫秒)
    uint32_t gpuDeviceId = 0;                                     ///< GPU设备ID
    uint32_t memoryPoolMb = 256;                                  ///< 内存池大小(MB)
};

/**
 * @brief 任务调度配置参数
 * @details 控制任务调度器的线程池和调度策略
 */
struct TaskSchedulerConfig {
    uint32_t coreThreads = 4;               ///< 核心线程数
    uint32_t maxThreads = 8;                ///< 最大线程数
    uint32_t queueCapacity = 500;           ///< 队列容量
    uint32_t keepAliveMs = 60000;           ///< 线程存活时间(毫秒)
    std::string schedulingPolicy = "fifo";  ///< 调度策略
    uint32_t maxRetryCount = 3;             ///< 最大重试次数
};

/**
 * @brief 显示格式枚举
 * @details 定义了不同的显示输出格式
 */
enum class DisplayFormat : uint8_t {
    TEXT = 0,  ///< 纯文本格式
    TABLE,     ///< 表格格式
    JSON,      ///< JSON格式
    CSV,       ///< CSV格式
    BINARY,    ///< 二进制格式
    GRAPHICAL  ///< 图形化格式
};

/**
 * @brief 显示控制器配置参数
 * @details 控制显示控制模块的输出格式和性能参数
 */
struct DisplayControllerConfig {
    DisplayFormat outputFormat = DisplayFormat::TEXT;  ///< 输出格式
    std::string outputPath = "./output";               ///< 输出路径
    uint32_t maxFrameRate = 30;                        ///< 最大帧率(FPS)
    uint32_t bufferSize = 100;                         ///< 缓冲区大小
    bool realTimeDisplay = true;                       ///< 是否实时显示
    uint32_t maxFileSize = 100 * 1024 * 1024;          ///< 最大文件大小(字节)
    bool compressionEnabled = false;                   ///< 是否启用压缩
    std::string timestampFormat = "ISO8601";           ///< 时间戳格式
};

/**
 * @brief 处理器能力信息
 * @details 描述数据处理器的能力和限制
 */
struct ProcessorCapabilities {
    bool supportsCPU = true;                              ///< 是否支持CPU处理
    bool supportsGPU = false;                             ///< 是否支持GPU加速
    uint32_t maxConcurrentTasks = 4;                      ///< 最大并发任务数
    uint64_t maxMemoryUsageMB = 1024;                     ///< 最大内存使用量(MB)
    std::vector<ProcessingStrategy> supportedStrategies;  ///< 支持的处理策略
    std::string processorInfo;                            ///< 处理器信息描述
};

/**
 * @brief 显示器能力信息
 * @details 描述显示控制器的能力和限制
 */
struct DisplayCapabilities {
    std::vector<DisplayFormat> supportedFormats;  ///< 支持的显示格式
    uint32_t maxFrameRate = 60;                   ///< 最大支持帧率
    bool supportsRealTime = true;                 ///< 是否支持实时显示
    bool supportsFileOutput = true;               ///< 是否支持文件输出
    bool supportsCompression = false;             ///< 是否支持压缩
    uint64_t maxBufferSizeMB = 512;               ///< 最大缓冲区大小(MB)
    std::string displayInfo;                      ///< 显示器信息描述
};

/**
 * @brief 调度器能力信息
 * @details 描述任务调度器的能力和限制
 */
struct SchedulerCapabilities {
    uint32_t maxThreads = 16;                      ///< 最大线程数
    uint32_t maxQueueSize = 10000;                 ///< 最大队列大小
    bool supportsRealTime = false;                 ///< 是否支持实时调度
    bool supportsPriority = true;                  ///< 是否支持优先级调度
    std::vector<std::string> supportedStrategies;  ///< 支持的调度策略
    std::string schedulerInfo;                     ///< 调度器信息描述
};

/**
 * @brief 调度器状态信息
 * @details 描述任务调度器的当前运行状态
 */
struct SchedulerStatus {
    uint32_t activeThreads = 0;                               ///< 活跃线程数
    uint32_t pendingTasks = 0;                                ///< 等待任务数
    uint32_t completedTasks = 0;                              ///< 已完成任务数
    uint32_t failedTasks = 0;                                 ///< 失败任务数
    double averageExecutionTimeMs = 0.0;                      ///< 平均执行时间（毫秒）
    double throughputTasksPerSec = 0.0;                       ///< 吞吐量（任务/秒）
    ModuleState schedulerState = ModuleState::UNINITIALIZED;  ///< 调度器状态
};

//==============================================================================
// 核心数据结构
//==============================================================================

/**
 * @brief 雷达原始数据包结构
 * @details 包含从雷达前端接收到的原始I/Q数据以及相关元信息
 * @note 结构体采用内存对齐优化，提高GPU处理性能
 */
struct alignas(16) RawDataPacket {
    Timestamp timestamp;         ///< 数据采集时间戳
    uint64_t sequenceId;         ///< 数据包序列号
    PacketPriority priority;     ///< 数据包优先级
    uint32_t channelCount;       ///< 通道数量
    uint32_t samplesPerChannel;  ///< 每通道采样点数

    AlignedComplexVector iqData;  ///< I/Q复数据

    /// 数据包元信息
    struct Metadata {
        double samplingFrequency;          ///< 采样频率(Hz)
        double centerFrequency;            ///< 中心频率(Hz)
        double gain;                       ///< 增益设置
        uint32_t pulseRepetitionInterval;  ///< 脉冲重复间隔
    } metadata;

    /**
     * @brief 检查数据包的有效性
     * @return 数据包是否有效
     * @retval true 数据包格式正确，可以处理
     * @retval false 数据包存在错误，需要丢弃
     */
    bool isValid() const {
        return !iqData.empty() && channelCount > 0 && samplesPerChannel > 0 &&
               iqData.size() == channelCount * samplesPerChannel;
    }

    /**
     * @brief 获取数据包大小（字节）
     * @return 数据包占用的内存大小
     */
    size_t getDataSize() const {
        return sizeof(*this) + iqData.size() * sizeof(ComplexFloat);
    }
};

/**
 * @brief 数据处理结果结构
 * @details 包含处理后的雷达数据和相关统计信息
 */
struct ProcessingResult {
    Timestamp processingTime;  ///< 处理完成时间戳
    uint64_t sourcePacketId;   ///< 源数据包ID
    bool processingSuccess;    ///< 处理是否成功

    /// 处理后的数据
    AlignedFloatVector rangeProfile;     ///< 距离剖面数据
    AlignedFloatVector dopplerSpectrum;  ///< 多普勒频谱数据
    AlignedFloatVector beamformedData;   ///< 波束形成数据

    /// 处理性能统计
    struct Statistics {
        double processingDurationMs;  ///< 处理耗时(毫秒)
        double cpuUsagePercent;       ///< CPU使用率
        double gpuUsagePercent;       ///< GPU使用率
        size_t memoryUsageBytes;      ///< 内存使用量(字节)
    } statistics;

    /**
     * @brief 检查处理结果的完整性
     * @return 结果是否完整有效
     */
    bool isComplete() const {
        return processingSuccess && !rangeProfile.empty() && !dopplerSpectrum.empty();
    }
};

/**
 * @brief 系统性能监控数据结构
 * @details 用于收集和报告系统整体性能指标
 */
struct SystemPerformanceMetrics {
    Timestamp measurementTime;  ///< 测量时间戳

    /// 模块性能指标
    struct ModuleMetrics {
        ModuleState state;          ///< 模块状态
        uint64_t packetsProcessed;  ///< 已处理数据包数
        uint64_t packetsDropped;    ///< 丢弃数据包数
        double averageLatencyMs;    ///< 平均延迟(毫秒)
        double throughputMbps;      ///< 吞吐量(Mbps)
    };

    ModuleMetrics dataReceiverMetrics;       ///< 数据接收模块指标
    ModuleMetrics dataProcessorMetrics;      ///< 数据处理模块指标
    ModuleMetrics taskSchedulerMetrics;      ///< 任务调度模块指标
    ModuleMetrics displayControllerMetrics;  ///< 显示控制模块指标

    /// 系统资源使用
    struct ResourceUsage {
        double cpuUsagePercent;   ///< 系统CPU使用率
        double memoryUsageMb;     ///< 系统内存使用量(MB)
        double gpuUsagePercent;   ///< GPU使用率
        double gpuMemoryUsageMb;  ///< GPU内存使用量(MB)
    } resourceUsage;
};

//==============================================================================
// 智能指针类型别名
//==============================================================================

/// 原始数据包智能指针
using RawDataPacketPtr = std::shared_ptr<RawDataPacket>;
using RawDataPacketUniquePtr = std::unique_ptr<RawDataPacket>;

/// 处理结果智能指针
using ProcessingResultPtr = std::shared_ptr<ProcessingResult>;
using ProcessingResultUniquePtr = std::unique_ptr<ProcessingResult>;

/// 性能指标智能指针
using PerformanceMetricsPtr = std::shared_ptr<SystemPerformanceMetrics>;

}  // namespace radar
