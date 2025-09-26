/**
 * @file radar_application.h
 * @brief 雷达MVP系统主应用程序控制器
 *
 * 本文件定义了雷达系统的主应用程序类，负责协调和管理整个系统的生命周期。
 * 采用外观模式和状态模式设计，提供统一的系统控制接口。
 *
 * 主要职责：
 * - 系统模块的初始化和生命周期管理
 * - 配置加载和管理
 * - 模块间的协调和通信
 * - 系统状态监控和异常处理
 * - 优雅关闭和资源清理
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see common::IModule
 * @see modules::IDataReceiver
 * @see modules::IDataProcessor
 * @see modules::ITaskScheduler
 * @see modules::IDisplayController
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/config_manager.h"
#include "common/error_codes.h"
#include "common/interfaces.h"
#include "common/logger.h"
#include "common/types.h"

namespace radar {
namespace application {

//==============================================================================
// 前向声明
//==============================================================================

class SystemStatusMonitor;
class ModuleCoordinator;
class ConfigurationManager;

//==============================================================================
// 应用程序状态枚举
//==============================================================================

/**
 * @brief 应用程序运行状态枚举
 *
 * 定义了应用程序的各种运行状态，用于状态机管理和外部查询。
 */
enum class ApplicationState : uint8_t {
    UNINITIALIZED = 0,  ///< 未初始化状态
    INITIALIZING,       ///< 正在初始化
    INITIALIZED,        ///< 已初始化，准备就绪
    STARTING,           ///< 正在启动
    RUNNING,            ///< 正常运行中
    PAUSING,            ///< 正在暂停
    PAUSED,             ///< 已暂停
    RESUMING,           ///< 正在恢复
    STOPPING,           ///< 正在停止
    STOPPED,            ///< 已停止
    APP_ERROR,          ///< 错误状态
    FATAL_ERROR         ///< 致命错误状态
};

/**
 * @brief 应用程序启动模式
 */
enum class LaunchMode : uint8_t {
    NORMAL = 0,  ///< 正常启动模式
    SIMULATION,  ///< 仿真模式
    DEBUG,       ///< 调试模式
    BENCHMARK,   ///< 性能测试模式
    RECOVERY     ///< 恢复模式
};

//==============================================================================
// 配置结构体定义
//==============================================================================

/**
 * @brief 应用程序配置参数
 *
 * 包含应用程序运行所需的各种配置参数。
 */
struct ApplicationConfig {
    /// 基础配置
    std::string applicationName = "RadarMVP";    ///< 应用程序名称
    std::string version = "1.0.0";               ///< 版本号
    LaunchMode launchMode = LaunchMode::NORMAL;  ///< 启动模式

    /// 运行时配置
    uint32_t maxRetryAttempts = 3;            ///< 最大重试次数
    uint32_t shutdownTimeoutMs = 30000;       ///< 关闭超时时间（毫秒）
    uint32_t heartbeatIntervalMs = 1000;      ///< 心跳间隔（毫秒）
    bool enablePerformanceMonitoring = true;  ///< 是否启用性能监控
    bool enableAutoRecovery = true;           ///< 是否启用自动恢复

    /// 模块配置文件路径
    std::string configFilePath = "./configs/config.yaml";                ///< 主配置文件路径
    std::string dataReceiverConfigPath = "./configs/receiver.yaml";      ///< 数据接收配置
    std::string dataProcessorConfigPath = "./configs/processor.yaml";    ///< 数据处理配置
    std::string taskSchedulerConfigPath = "./configs/scheduler.yaml";    ///< 任务调度配置
    std::string displayControllerConfigPath = "./configs/display.yaml";  ///< 显示控制配置

    /// 日志配置
    std::string logLevel = "INFO";        ///< 日志级别
    std::string logFilePath = "./logs/";  ///< 日志文件路径
    bool enableConsoleOutput = true;      ///< 是否启用控制台输出
    bool enableFileOutput = true;         ///< 是否启用文件输出
};

/**
 * @brief 系统性能指标
 */
struct SystemMetrics {
    /// 系统资源使用情况
    std::atomic<double> cpuUsagePercent{0.0};  ///< CPU使用率（百分比）
    std::atomic<double> memoryUsageMB{0.0};    ///< 内存使用量（MB）
    std::atomic<double> diskUsageMB{0.0};      ///< 磁盘使用量（MB）

    /// 数据处理指标
    std::atomic<uint64_t> totalPacketsProcessed{0};  ///< 总处理数据包数
    std::atomic<uint64_t> packetsPerSecond{0};       ///< 每秒处理数据包数
    std::atomic<double> averageLatencyMs{0.0};       ///< 平均延迟（毫秒）
    std::atomic<double> throughputMbps{0.0};         ///< 吞吐量（Mbps）

    /// 错误统计
    std::atomic<uint32_t> totalErrors{0};     ///< 总错误数
    std::atomic<uint32_t> criticalErrors{0};  ///< 关键错误数
    std::atomic<uint32_t> warningCount{0};    ///< 警告数

    /// 时间信息
    Timestamp startTime;                     ///< 启动时间
    Timestamp lastUpdateTime;                ///< 最后更新时间
    std::atomic<uint64_t> uptimeSeconds{0};  ///< 运行时间（秒）

    /**
     * @brief 重置所有性能指标到初始状态
     */
    void reset() {
        cpuUsagePercent.store(0.0);
        memoryUsageMB.store(0.0);
        diskUsageMB.store(0.0);
        totalPacketsProcessed.store(0);
        packetsPerSecond.store(0);
        averageLatencyMs.store(0.0);
        throughputMbps.store(0.0);
        totalErrors.store(0);
        criticalErrors.store(0);
        warningCount.store(0);
        uptimeSeconds.store(0);
        startTime = std::chrono::high_resolution_clock::now();
        lastUpdateTime = startTime;
    }

    /**
     * @brief 从另一个SystemMetrics对象复制值
     */
    void copyFrom(const SystemMetrics &other) {
        cpuUsagePercent.store(other.cpuUsagePercent.load());
        memoryUsageMB.store(other.memoryUsageMB.load());
        diskUsageMB.store(other.diskUsageMB.load());
        totalPacketsProcessed.store(other.totalPacketsProcessed.load());
        packetsPerSecond.store(other.packetsPerSecond.load());
        averageLatencyMs.store(other.averageLatencyMs.load());
        throughputMbps.store(other.throughputMbps.load());
        totalErrors.store(other.totalErrors.load());
        criticalErrors.store(other.criticalErrors.load());
        warningCount.store(other.warningCount.load());
        uptimeSeconds.store(other.uptimeSeconds.load());
        startTime = other.startTime;
        lastUpdateTime = other.lastUpdateTime;
    }
};

//==============================================================================
// 回调函数类型定义
//==============================================================================

/// 状态变化回调函数
using ApplicationStateChangeCallback = std::function<void(ApplicationState, ApplicationState)>;

/// 模块错误回调函数
using ModuleErrorCallback = std::function<void(const std::string &, ErrorCode, const std::string &)>;

/// 性能指标更新回调函数
using PerformanceMetricsCallback = std::function<void(const SystemMetrics &)>;

/// 系统事件回调函数
using SystemEventCallback = std::function<void(const std::string &, const std::string &)>;

//==============================================================================
// 主应用程序类
//==============================================================================

/**
 * @brief 雷达MVP系统主应用程序控制器
 *
 * 该类是整个雷达系统的核心控制器，采用外观模式设计，为外部提供统一的
 * 系统控制接口。内部管理所有功能模块的生命周期，协调模块间的交互。
 *
 * @details
 * 设计模式应用：
 * - 外观模式：为复杂的子系统提供简化的接口
 * - 状态模式：管理应用程序的各种运行状态
 * - 观察者模式：状态变化和事件通知机制
 * - 策略模式：支持不同的启动和运行策略
 * - 单例模式：确保整个系统只有一个应用程序实例
 *
 * 使用流程：
 * 1. 创建 RadarApplication 实例
 * 2. 调用 configure() 配置应用程序参数
 * 3. 调用 initialize() 初始化所有模块
 * 4. 调用 start() 启动系统运行
 * 5. 监控系统状态和性能指标
 * 6. 调用 stop() 停止系统运行
 * 7. 调用 shutdown() 清理资源
 *
 * @note 该类的所有公共方法都是线程安全的
 * @warning 必须按照指定的生命周期顺序调用方法
 * @see IModule
 * @see ApplicationConfig
 * @see SystemMetrics
 */
class RadarApplication {
  public:
    /**
     * @brief 默认构造函数
     *
     * 创建应用程序实例，初始化基本状态和日志系统。
     */
    RadarApplication();

    /**
     * @brief 析构函数
     *
     * 自动清理所有资源，确保优雅关闭。
     * 如果系统仍在运行，会自动调用 shutdown()。
     */
    virtual ~RadarApplication();

    // 禁用拷贝构造和赋值（单例模式）
    RadarApplication(const RadarApplication &) = delete;
    RadarApplication &operator=(const RadarApplication &) = delete;

    // 支持移动构造和赋值
    RadarApplication(RadarApplication &&other) noexcept;
    RadarApplication &operator=(RadarApplication &&other) noexcept;

    //==============================================================================
    // 生命周期管理接口
    //==============================================================================

    /**
     * @brief 配置应用程序参数
     *
     * 设置应用程序的运行参数和各模块的配置。
     * 该方法必须在 initialize() 之前调用。
     *
     * @param config 应用程序配置参数
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 配置成功
     * @retval SystemErrors::INVALID_PARAMETER 配置参数无效
     * @retval SystemErrors::CONFIGURATION_ERROR 配置文件错误
     *
     * @note 该方法只能在 UNINITIALIZED 状态下调用
     * @warning 配置的有效性会在 initialize() 时进行验证
     */
    ErrorCode configure(const ApplicationConfig &config);

    /**
     * @brief 从配置文件加载配置
     *
     * 从指定的YAML配置文件加载应用程序配置。
     *
     * @param configFilePath 配置文件路径
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 加载成功
     * @retval SystemErrors::CONFIGURATION_ERROR 配置文件错误
     *
     * @see configure()
     */
    ErrorCode loadConfiguration(const std::string &configFilePath);

    /**
     * @brief 初始化应用程序
     *
     * 初始化所有功能模块，建立模块间的连接关系。
     * 该方法会验证配置的有效性并分配必要的资源。
     *
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 初始化成功
     * @retval SystemErrors::INITIALIZATION_FAILED 初始化失败
     * @retval SystemErrors::CONFIGURATION_ERROR 配置错误
     * @retval SystemErrors::RESOURCE_UNAVAILABLE 资源不可用
     *
     * @note 该方法只能在 UNINITIALIZED 状态下调用
     * @warning 初始化失败时需要调用 shutdown() 清理资源
     */
    ErrorCode initialize();

    /**
     * @brief 启动系统运行
     *
     * 启动所有模块并开始数据处理流程。
     * 系统进入正常运行状态，开始接收和处理数据。
     *
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 启动成功
     * @retval SystemErrors::INITIALIZATION_FAILED 模块启动失败
     * @retval SystemErrors::RESOURCE_UNAVAILABLE 资源不可用
     *
     * @note 该方法只能在 INITIALIZED 或 STOPPED 状态下调用
     */
    ErrorCode start();

    /**
     * @brief 暂停系统运行
     *
     * 暂停数据处理，但保持模块连接状态。
     * 可以通过 resume() 恢复运行。
     *
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 暂停成功
     *
     * @note 该方法只能在 RUNNING 状态下调用
     * @see resume()
     */
    ErrorCode pause();

    /**
     * @brief 恢复系统运行
     *
     * 从暂停状态恢复正常运行。
     *
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 恢复成功
     *
     * @note 该方法只能在 PAUSED 状态下调用
     * @see pause()
     */
    ErrorCode resume();

    /**
     * @brief 停止系统运行
     *
     * 停止数据处理并断开模块连接，但保持模块初始化状态。
     * 可以通过 start() 重新启动。
     *
     * @param timeoutMs 停止超时时间（毫秒），0表示无限等待
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 停止成功
     * @retval SystemErrors::OPERATION_TIMEOUT 停止超时
     *
     * @note 该方法可在任何运行状态下调用
     */
    ErrorCode stop(uint32_t timeoutMs = 0);

    /**
     * @brief 关闭应用程序
     *
     * 完全关闭系统，清理所有资源。
     * 该方法调用后，应用程序需要重新初始化才能使用。
     *
     * @param timeoutMs 关闭超时时间（毫秒），0表示无限等待
     * @return 操作结果错误码
     * @retval SystemErrors::SUCCESS 关闭成功
     * @retval SystemErrors::SHUTDOWN_FAILED 关闭失败
     *
     * @note 该方法可在任何状态下调用，会强制清理资源
     * @warning 关闭后的实例不能重新使用
     */
    ErrorCode shutdown(uint32_t timeoutMs = 0);

    //==============================================================================
    // 状态查询接口
    //==============================================================================

    /**
     * @brief 获取当前应用程序状态
     *
     * @return 当前应用程序状态
     */
    ApplicationState getState() const;

    /**
     * @brief 检查应用程序是否正在运行
     *
     * @return 是否正在运行
     * @retval true 系统正在运行（RUNNING 状态）
     * @retval false 系统未运行或处于其他状态
     */
    bool isRunning() const;

    /**
     * @brief 检查应用程序是否已初始化
     *
     * @return 是否已初始化
     * @retval true 系统已初始化
     * @retval false 系统未初始化
     */
    bool isInitialized() const;

    /**
     * @brief 检查应用程序是否处于错误状态
     *
     * @return 是否处于错误状态
     */
    bool hasError() const;

    /**
     * @brief 获取最后发生的错误信息
     *
     * @return 错误信息描述
     */
    std::string getLastErrorMessage() const;

    //==============================================================================
    // 配置和模块管理接口
    //==============================================================================

    /**
     * @brief 获取当前应用程序配置
     *
     * @return 应用程序配置的常量引用
     */
    const ApplicationConfig &getConfiguration() const;

    /**
     * @brief 获取指定模块的引用
     *
     * @tparam T 模块接口类型
     * @param moduleName 模块名称
     * @return 模块的智能指针
     * @throws std::invalid_argument 模块不存在或类型错误
     *
     * @note 返回的指针可能为空，调用前需要检查
     */
    template <typename T>
    std::shared_ptr<T> getModule(const std::string &moduleName) const;

    /**
     * @brief 获取数据接收器模块
     *
     * @return 数据接收器模块的智能指针
     */
    std::shared_ptr<IDataReceiver> getDataReceiver() const;

    /**
     * @brief 获取数据处理器模块
     *
     * @return 数据处理器模块的智能指针
     */
    std::shared_ptr<IDataProcessor> getDataProcessor() const;

    /**
     * @brief 获取任务调度器模块
     *
     * @return 任务调度器模块的智能指针
     */
    std::shared_ptr<ITaskScheduler> getTaskScheduler() const;

    /**
     * @brief 获取显示控制器模块
     *
     * @return 显示控制器模块的智能指针
     */
    std::shared_ptr<IDisplayController> getDisplayController() const;

    /**
     * @brief 获取所有模块的状态
     *
     * @return 模块名称到状态的映射
     */
    std::unordered_map<std::string, ModuleState> getModuleStates() const;

    //==============================================================================
    // 性能监控接口
    //==============================================================================

    /**
     * @brief 获取系统性能指标
     *
     * @return 系统性能指标的常量引用
     */
    const SystemMetrics &getSystemMetrics() const;

    /**
     * @brief 获取系统运行时长
     *
     * @return 运行时长（秒）
     */
    uint64_t getUptimeSeconds() const;

    /**
     * @brief 重置性能统计信息
     *
     * 清零所有性能计数器，重新开始统计。
     */
    void resetMetrics();

    /**
     * @brief 获取模块性能指标
     *
     * @param moduleName 模块名称
     * @return 模块性能指标的智能指针
     */
    PerformanceMetricsPtr getModuleMetrics(const std::string &moduleName) const;

    //==============================================================================
    // 回调函数管理接口
    //==============================================================================

    /**
     * @brief 设置状态变化回调函数
     *
     * @param callback 状态变化回调函数
     * @return 回调函数ID，用于后续移除
     */
    uint32_t setStateChangeCallback(ApplicationStateChangeCallback callback);

    /**
     * @brief 设置模块错误回调函数
     *
     * @param callback 模块错误回调函数
     * @return 回调函数ID
     */
    uint32_t setModuleErrorCallback(ModuleErrorCallback callback);

    /**
     * @brief 设置性能指标更新回调函数
     *
     * @param callback 性能指标更新回调函数
     * @return 回调函数ID
     */
    uint32_t setPerformanceMetricsCallback(PerformanceMetricsCallback callback);

    /**
     * @brief 设置系统事件回调函数
     *
     * @param callback 系统事件回调函数
     * @return 回调函数ID
     */
    uint32_t setSystemEventCallback(SystemEventCallback callback);

    /**
     * @brief 移除回调函数
     *
     * @param callbackId 回调函数ID
     * @return 是否成功移除
     */
    bool removeCallback(uint32_t callbackId);

    //==============================================================================
    // 工具方法
    //==============================================================================

    /**
     * @brief 获取应用程序版本信息
     *
     * @return 版本信息字符串
     */
    std::string getVersionInfo() const;

    /**
     * @brief 获取系统信息摘要
     *
     * @return 系统信息的JSON格式字符串
     */
    std::string getSystemInfo() const;

    /**
     * @brief 导出配置到文件
     *
     * @param filePath 导出文件路径
     * @return 操作结果错误码
     */
    ErrorCode exportConfiguration(const std::string &filePath) const;

    /**
     * @brief 验证系统配置
     *
     * @param errorReport 错误报告输出
     * @return 配置是否有效
     */
    bool validateConfiguration(std::vector<std::string> &errorReport) const;

  protected:
    //==============================================================================
    // 内部实现方法
    //==============================================================================

    /**
     * @brief 初始化日志系统
     *
     * @return 操作结果错误码
     */
    ErrorCode initializeLogging();

    /**
     * @brief 初始化配置管理器
     *
     * @return 操作结果错误码
     */
    ErrorCode initializeConfigurationManager();

    /**
     * @brief 创建并初始化所有模块
     *
     * @return 操作结果错误码
     */
    ErrorCode initializeModules();

    /**
     * @brief 建立模块间的连接
     *
     * @return 操作结果错误码
     */
    ErrorCode connectModules();

    /**
     * @brief 启动性能监控
     *
     * @return 操作结果错误码
     */
    ErrorCode startPerformanceMonitoring();

    /**
     * @brief 停止性能监控
     */
    void stopPerformanceMonitoring();

    /**
     * @brief 设置应用程序状态
     *
     * @param newState 新状态
     */
    void setState(ApplicationState newState);

    /**
     * @brief 通知状态变化
     *
     * @param oldState 旧状态
     * @param newState 新状态
     */
    void notifyStateChange(ApplicationState oldState, ApplicationState newState);

    /**
     * @brief 处理模块错误
     *
     * @param moduleName 模块名称
     * @param errorCode 错误码
     * @param errorMessage 错误信息
     */
    void handleModuleError(const std::string &moduleName, ErrorCode errorCode, const std::string &errorMessage);

    /**
     * @brief 更新性能指标
     */
    void updatePerformanceMetrics();

    /**
     * @brief 性能监控线程主循环
     */
    void performanceMonitoringLoop();

    /**
     * @brief 心跳检查线程主循环
     */
    void heartbeatLoop();

    /**
     * @brief 验证应用程序状态转换
     *
     * @param fromState 源状态
     * @param toState 目标状态
     * @return 状态转换是否有效
     */
    bool isValidStateTransition(ApplicationState fromState, ApplicationState toState) const;

    /**
     * @brief 清理所有资源
     */
    void cleanup();

  private:
    //==============================================================================
    // 成员变量
    //==============================================================================

    /// 应用程序状态
    mutable std::mutex stateMutex_;               ///< 状态访问互斥锁
    std::atomic<ApplicationState> currentState_;  ///< 当前应用程序状态

    /// 配置和管理器
    ApplicationConfig config_;                              ///< 应用程序配置
    std::shared_ptr<spdlog::logger> logger_;                ///< 日志记录器
    std::shared_ptr<common::ConfigManager> configManager_;  ///< 配置管理器

    /// 模块管理
    mutable std::mutex modulesMutex_;                                    ///< 模块访问互斥锁
    std::unordered_map<std::string, std::shared_ptr<IModule>> modules_;  ///< 模块映射表

    std::shared_ptr<IDataReceiver> dataReceiver_;            ///< 数据接收器
    std::shared_ptr<IDataProcessor> dataProcessor_;          ///< 数据处理器
    std::shared_ptr<ITaskScheduler> taskScheduler_;          ///< 任务调度器
    std::shared_ptr<IDisplayController> displayController_;  ///< 显示控制器

    /// 性能监控
    mutable std::mutex metricsMutex_;       ///< 性能指标互斥锁
    SystemMetrics systemMetrics_;           ///< 系统性能指标
    std::thread performanceMonitorThread_;  ///< 性能监控线程
    std::thread heartbeatThread_;           ///< 心跳检查线程
    std::atomic<bool> monitoringActive_;    ///< 监控活跃标志

    /// 回调管理
    mutable std::mutex callbacksMutex_;                                                  ///< 回调函数互斥锁
    std::atomic<uint32_t> nextCallbackId_;                                               ///< 下一个回调ID
    std::unordered_map<uint32_t, ApplicationStateChangeCallback> stateChangeCallbacks_;  ///< 状态变化回调
    std::unordered_map<uint32_t, ModuleErrorCallback> moduleErrorCallbacks_;             ///< 模块错误回调
    std::unordered_map<uint32_t, PerformanceMetricsCallback> metricsCallbacks_;          ///< 性能指标回调
    std::unordered_map<uint32_t, SystemEventCallback> systemEventCallbacks_;             ///< 系统事件回调

    /// 错误处理
    mutable std::mutex errorMutex_;  ///< 错误信息互斥锁
    ErrorCode lastErrorCode_;        ///< 最后的错误码
    std::string lastErrorMessage_;   ///< 最后的错误信息

    /// 同步控制
    std::condition_variable shutdownCondition_;  ///< 关闭条件变量
    mutable std::mutex shutdownMutex_;           ///< 关闭互斥锁
    std::atomic<bool> shutdownRequested_;        ///< 关闭请求标志
};

//==============================================================================
// 工厂函数和工具函数
//==============================================================================

/**
 * @brief 创建雷达应用程序实例
 *
 * @param config 应用程序配置（可选）
 * @return 应用程序实例的智能指针
 */
std::unique_ptr<RadarApplication> createRadarApplication(const ApplicationConfig &config = ApplicationConfig{});

/**
 * @brief 获取应用程序状态的字符串表示
 *
 * @param state 应用程序状态
 * @return 状态名称字符串
 */
const char *getApplicationStateString(ApplicationState state);

/**
 * @brief 获取启动模式的字符串表示
 *
 * @param mode 启动模式
 * @return 模式名称字符串
 */
const char *getLaunchModeString(LaunchMode mode);

/**
 * @brief 加载默认应用程序配置
 *
 * @return 默认配置对象
 */
ApplicationConfig loadDefaultConfiguration();

//==============================================================================
// 模板方法实现
//==============================================================================

template <typename T>
std::shared_ptr<T> RadarApplication::getModule(const std::string &moduleName) const {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        return nullptr;
    }

    return std::dynamic_pointer_cast<T>(it->second);
}

}  // namespace application
}  // namespace radar
