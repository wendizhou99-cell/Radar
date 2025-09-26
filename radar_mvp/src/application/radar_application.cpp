/**
 * @file radar_application.cpp
 * @brief 雷达MVP系统主应用程序控制器实现
 *
 * 本文件提供了 RadarApplication 类的具体实现，包括所有生命周期管理、
 * 模块协调、性能监控和错误处理功能的实现。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see radar_application.h
 */

#include "application/radar_application.h"
// #include "modules/data_receiver.h"
// #include "modules/data_processor.h"
// #include "modules/task_scheduler.h"
// #include "modules/display_controller.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace radar {
namespace application {

//==============================================================================
// 静态工具函数实现
//==============================================================================

const char *getApplicationStateString(ApplicationState state) {
    switch (state) {
        case ApplicationState::UNINITIALIZED:
            return "UNINITIALIZED";
        case ApplicationState::INITIALIZING:
            return "INITIALIZING";
        case ApplicationState::INITIALIZED:
            return "INITIALIZED";
        case ApplicationState::STARTING:
            return "STARTING";
        case ApplicationState::RUNNING:
            return "RUNNING";
        case ApplicationState::PAUSING:
            return "PAUSING";
        case ApplicationState::PAUSED:
            return "PAUSED";
        case ApplicationState::RESUMING:
            return "RESUMING";
        case ApplicationState::STOPPING:
            return "STOPPING";
        case ApplicationState::STOPPED:
            return "STOPPED";
        case ApplicationState::APP_ERROR:
            return "APP_ERROR";
        case ApplicationState::FATAL_ERROR:
            return "FATAL_ERROR";
        default:
            return "UNKNOWN";
    }
}

const char *getLaunchModeString(LaunchMode mode) {
    switch (mode) {
        case LaunchMode::NORMAL:
            return "NORMAL";
        case LaunchMode::SIMULATION:
            return "SIMULATION";
        case LaunchMode::DEBUG:
            return "DEBUG";
        case LaunchMode::BENCHMARK:
            return "BENCHMARK";
        case LaunchMode::RECOVERY:
            return "RECOVERY";
        default:
            return "UNKNOWN";
    }
}

ApplicationConfig loadDefaultConfiguration() {
    ApplicationConfig config;
    // 使用默认值，已在结构体定义中设置
    return config;
}

std::unique_ptr<RadarApplication> createRadarApplication(const ApplicationConfig &config) {
    auto app = std::make_unique<RadarApplication>();
    if (app) {
        auto result = app->configure(config);
        if (result != SystemErrors::SUCCESS) {
            // 配置失败，返回空指针
            return nullptr;
        }
    }
    return app;
}

//==============================================================================
// RadarApplication 类实现
//==============================================================================

RadarApplication::RadarApplication()
    : currentState_(ApplicationState::UNINITIALIZED),
      logger_(nullptr),
      configManager_(nullptr),
      monitoringActive_(false),
      nextCallbackId_(1),
      lastErrorCode_(SystemErrors::SUCCESS),
      shutdownRequested_(false) {
    systemMetrics_.startTime = std::chrono::high_resolution_clock::now();
    systemMetrics_.lastUpdateTime = systemMetrics_.startTime;
}

RadarApplication::~RadarApplication() {
    // 确保优雅关闭
    if (currentState_.load() != ApplicationState::UNINITIALIZED) {
        shutdown(config_.shutdownTimeoutMs);
    }
}

RadarApplication::RadarApplication(RadarApplication &&other) noexcept
    : currentState_(other.currentState_.load()),
      config_(std::move(other.config_)),
      logger_(std::move(other.logger_)),
      configManager_(std::move(other.configManager_)),
      modules_(std::move(other.modules_)),
      dataReceiver_(std::move(other.dataReceiver_)),
      dataProcessor_(std::move(other.dataProcessor_)),
      taskScheduler_(std::move(other.taskScheduler_)),
      displayController_(std::move(other.displayController_)),
      performanceMonitorThread_(std::move(other.performanceMonitorThread_)),
      heartbeatThread_(std::move(other.heartbeatThread_)),
      monitoringActive_(other.monitoringActive_.load()),
      nextCallbackId_(other.nextCallbackId_.load()),
      stateChangeCallbacks_(std::move(other.stateChangeCallbacks_)),
      moduleErrorCallbacks_(std::move(other.moduleErrorCallbacks_)),
      metricsCallbacks_(std::move(other.metricsCallbacks_)),
      systemEventCallbacks_(std::move(other.systemEventCallbacks_)),
      lastErrorCode_(other.lastErrorCode_),
      lastErrorMessage_(std::move(other.lastErrorMessage_)),
      shutdownRequested_(other.shutdownRequested_.load()) {
    // 手动复制SystemMetrics
    systemMetrics_.copyFrom(other.systemMetrics_);

    other.currentState_ = ApplicationState::UNINITIALIZED;
}

RadarApplication &RadarApplication::operator=(RadarApplication &&other) noexcept {
    if (this != &other) {
        // 先关闭当前实例
        shutdown(config_.shutdownTimeoutMs);

        // 移动赋值
        currentState_ = other.currentState_.load();
        config_ = std::move(other.config_);
        logger_ = std::move(other.logger_);
        configManager_ = std::move(other.configManager_);
        modules_ = std::move(other.modules_);
        dataReceiver_ = std::move(other.dataReceiver_);
        dataProcessor_ = std::move(other.dataProcessor_);
        taskScheduler_ = std::move(other.taskScheduler_);
        displayController_ = std::move(other.displayController_);

        // 手动复制SystemMetrics
        systemMetrics_.copyFrom(other.systemMetrics_);

        performanceMonitorThread_ = std::move(other.performanceMonitorThread_);
        heartbeatThread_ = std::move(other.heartbeatThread_);
        monitoringActive_ = other.monitoringActive_.load();
        nextCallbackId_ = other.nextCallbackId_.load();
        stateChangeCallbacks_ = std::move(other.stateChangeCallbacks_);
        moduleErrorCallbacks_ = std::move(other.moduleErrorCallbacks_);
        metricsCallbacks_ = std::move(other.metricsCallbacks_);
        systemEventCallbacks_ = std::move(other.systemEventCallbacks_);
        lastErrorCode_ = other.lastErrorCode_;
        lastErrorMessage_ = std::move(other.lastErrorMessage_);
        shutdownRequested_ = other.shutdownRequested_.load();

        other.currentState_ = ApplicationState::UNINITIALIZED;
    }
    return *this;
}

//==============================================================================
// 生命周期管理接口实现（基本框架）
//==============================================================================

ErrorCode RadarApplication::configure(const ApplicationConfig &config) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (currentState_.load() != ApplicationState::UNINITIALIZED) {
        lastErrorMessage_ = "Configuration can only be done in UNINITIALIZED state";
        lastErrorCode_ = SystemErrors::INVALID_PARAMETER;
        return lastErrorCode_;
    }

    config_ = config;

    // 验证配置的基本有效性
    std::vector<std::string> errorReport;
    if (!validateConfiguration(errorReport)) {
        std::ostringstream oss;
        oss << "Configuration validation failed: ";
        for (const auto &error : errorReport) {
            oss << error << "; ";
        }
        lastErrorMessage_ = oss.str();
        lastErrorCode_ = SystemErrors::CONFIGURATION_ERROR;
        return lastErrorCode_;
    }

    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::loadConfiguration(const std::string &configFilePath) {
    // 使用[[maybe_unused]]属性标记暂时未使用的参数，避免编译警告
    [[maybe_unused]] const auto &path = configFilePath;

    // TODO: 实现从YAML文件加载配置的逻辑
    // 这里需要使用 yaml-cpp 库解析配置文件
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::initialize() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (currentState_.load() != ApplicationState::UNINITIALIZED) {
        lastErrorMessage_ = "Application already initialized";
        lastErrorCode_ = SystemErrors::INVALID_PARAMETER;
        return lastErrorCode_;
    }

    setState(ApplicationState::INITIALIZING);

    // TODO: 完整的初始化流程将在后续实现
    // 1. 初始化日志系统
    // 2. 初始化配置管理器
    // 3. 初始化所有模块
    // 4. 建立模块间连接
    // 5. 启动性能监控

    setState(ApplicationState::INITIALIZED);
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::start() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    auto currentState = currentState_.load();
    if (currentState != ApplicationState::INITIALIZED && currentState != ApplicationState::STOPPED) {
        lastErrorMessage_ = "Can only start from INITIALIZED or STOPPED state";
        lastErrorCode_ = SystemErrors::INVALID_PARAMETER;
        return lastErrorCode_;
    }

    setState(ApplicationState::STARTING);
    // TODO: 启动所有模块
    setState(ApplicationState::RUNNING);
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::pause() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (currentState_.load() != ApplicationState::RUNNING) {
        lastErrorMessage_ = "Can only pause from RUNNING state";
        lastErrorCode_ = SystemErrors::INVALID_PARAMETER;
        return lastErrorCode_;
    }

    setState(ApplicationState::PAUSED);
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::resume() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (currentState_.load() != ApplicationState::PAUSED) {
        lastErrorMessage_ = "Can only resume from PAUSED state";
        lastErrorCode_ = SystemErrors::INVALID_PARAMETER;
        return lastErrorCode_;
    }

    setState(ApplicationState::RUNNING);
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::stop(uint32_t timeoutMs) {
    [[maybe_unused]] const auto timeout = timeoutMs;

    std::lock_guard<std::mutex> lock(stateMutex_);
    setState(ApplicationState::STOPPED);
    return SystemErrors::SUCCESS;
}

ErrorCode RadarApplication::shutdown(uint32_t timeoutMs) {
    [[maybe_unused]] const auto timeout = timeoutMs;

    shutdownRequested_ = true;
    cleanup();
    setState(ApplicationState::UNINITIALIZED);
    return SystemErrors::SUCCESS;
}

//==============================================================================
// 状态查询接口实现
//==============================================================================

ApplicationState RadarApplication::getState() const {
    return currentState_.load();
}

bool RadarApplication::isRunning() const {
    return currentState_.load() == ApplicationState::RUNNING;
}

bool RadarApplication::isInitialized() const {
    auto state = currentState_.load();
    return state >= ApplicationState::INITIALIZED && state < ApplicationState::APP_ERROR;
}

bool RadarApplication::hasError() const {
    auto state = currentState_.load();
    return state == ApplicationState::APP_ERROR || state == ApplicationState::FATAL_ERROR;
}

std::string RadarApplication::getLastErrorMessage() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return lastErrorMessage_;
}

//==============================================================================
// 配置和模块管理接口实现
//==============================================================================

const ApplicationConfig &RadarApplication::getConfiguration() const {
    return config_;
}

std::shared_ptr<IDataReceiver> RadarApplication::getDataReceiver() const {
    return dataReceiver_;
}

std::shared_ptr<IDataProcessor> RadarApplication::getDataProcessor() const {
    return dataProcessor_;
}

std::shared_ptr<ITaskScheduler> RadarApplication::getTaskScheduler() const {
    return taskScheduler_;
}

std::shared_ptr<IDisplayController> RadarApplication::getDisplayController() const {
    return displayController_;
}

std::unordered_map<std::string, ModuleState> RadarApplication::getModuleStates() const {
    std::lock_guard<std::mutex> lock(modulesMutex_);
    std::unordered_map<std::string, ModuleState> states;

    for (const auto &[name, module] : modules_) {
        if (module) {
            states[name] = module->getState();
        }
    }

    return states;
}

//==============================================================================
// 其他接口的基础实现（完整版本待后续补充）
//==============================================================================

const SystemMetrics &RadarApplication::getSystemMetrics() const {
    return systemMetrics_;
}

uint64_t RadarApplication::getUptimeSeconds() const {
    return systemMetrics_.uptimeSeconds.load();
}

void RadarApplication::resetMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    systemMetrics_.reset();
}

PerformanceMetricsPtr RadarApplication::getModuleMetrics(const std::string &moduleName) const {
    auto module = getModule<IModule>(moduleName);
    if (module) {
        return module->getPerformanceMetrics();
    }
    return nullptr;
}

uint32_t RadarApplication::setStateChangeCallback(ApplicationStateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    uint32_t id = nextCallbackId_++;
    stateChangeCallbacks_[id] = callback;
    return id;
}

uint32_t RadarApplication::setModuleErrorCallback(ModuleErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    uint32_t id = nextCallbackId_++;
    moduleErrorCallbacks_[id] = callback;
    return id;
}

uint32_t RadarApplication::setPerformanceMetricsCallback(PerformanceMetricsCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    uint32_t id = nextCallbackId_++;
    metricsCallbacks_[id] = callback;
    return id;
}

uint32_t RadarApplication::setSystemEventCallback(SystemEventCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    uint32_t id = nextCallbackId_++;
    systemEventCallbacks_[id] = callback;
    return id;
}

bool RadarApplication::removeCallback(uint32_t callbackId) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);

    bool removed = false;
    removed |= (stateChangeCallbacks_.erase(callbackId) > 0);
    removed |= (moduleErrorCallbacks_.erase(callbackId) > 0);
    removed |= (metricsCallbacks_.erase(callbackId) > 0);
    removed |= (systemEventCallbacks_.erase(callbackId) > 0);

    return removed;
}

std::string RadarApplication::getVersionInfo() const {
    return config_.applicationName + " v" + config_.version;
}

std::string RadarApplication::getSystemInfo() const {
    // TODO: 实现系统信息的JSON序列化
    return "{}";
}

ErrorCode RadarApplication::exportConfiguration(const std::string &filePath) const {
    [[maybe_unused]] const auto &path = filePath;

    // TODO: 实现配置导出到YAML文件
    return SystemErrors::SUCCESS;
}

bool RadarApplication::validateConfiguration(std::vector<std::string> &errorReport) const {
    errorReport.clear();
    bool isValid = true;

    // 验证基本配置
    if (config_.applicationName.empty()) {
        errorReport.push_back("Application name cannot be empty");
        isValid = false;
    }

    if (config_.version.empty()) {
        errorReport.push_back("Version cannot be empty");
        isValid = false;
    }

    if (config_.shutdownTimeoutMs == 0) {
        errorReport.push_back("Shutdown timeout must be greater than 0");
        isValid = false;
    }

    return isValid;
}

//==============================================================================
// 内部实现方法（基础框架）
//==============================================================================

void RadarApplication::setState(ApplicationState newState) {
    auto oldState = currentState_.exchange(newState);
    if (oldState != newState) {
        notifyStateChange(oldState, newState);
    }
}

void RadarApplication::notifyStateChange(ApplicationState oldState, ApplicationState newState) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    for (const auto &[id, callback] : stateChangeCallbacks_) {
        try {
            callback(oldState, newState);
        } catch (const std::exception &e) {
            // 记录回调错误，但不影响状态变化
        }
    }
}

void RadarApplication::handleModuleError(const std::string &moduleName, ErrorCode errorCode,
                                         const std::string &errorMessage) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    for (const auto &[id, callback] : moduleErrorCallbacks_) {
        try {
            callback(moduleName, errorCode, errorMessage);
        } catch (const std::exception &e) {
            // 记录回调错误
        }
    }
}

void RadarApplication::cleanup() {
    // 清理所有模块
    if (dataReceiver_) {
        dataReceiver_.reset();
    }

    if (dataProcessor_) {
        dataProcessor_.reset();
    }

    if (taskScheduler_) {
        taskScheduler_.reset();
    }

    if (displayController_) {
        displayController_.reset();
    }

    modules_.clear();

    // 清理回调函数
    {
        std::lock_guard<std::mutex> lock(callbacksMutex_);
        stateChangeCallbacks_.clear();
        moduleErrorCallbacks_.clear();
        metricsCallbacks_.clear();
        systemEventCallbacks_.clear();
    }
}

}  // namespace application
}  // namespace radar
