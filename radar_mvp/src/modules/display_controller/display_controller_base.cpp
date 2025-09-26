/**
 * @file display_controller_base.cpp
 * @brief 显示控制器基类实现
 *
 * 实现了DisplayControllerBase类的基础功能。
 * 严格遵循接口合规性要求：
 * - 继承自IDisplayController接口
 * - 使用项目定义的数据类型
 * - 使用正确的成员变量名称
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

// 防止Windows宏定义与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif

#include "modules/display_controller/display_controller_base.h"

#include "common/logger.h"

namespace radar {
namespace modules {

//==============================================================================
// 构造函数和析构函数
//==============================================================================

DisplayControllerBase::DisplayControllerBase(const std::string &name)
    : state_(ModuleState::UNINITIALIZED),
      moduleName_(name),
      running_(false),
      shouldStop_(false),
      totalFramesDisplayed_(0),
      totalFramesDropped_(0),
      currentFrameRate_(0),
      updateIntervalMs_(100),
      autoRefreshEnabled_(false) {
    RADAR_INFO("DisplayControllerBase '{}' 创建", moduleName_);
}

DisplayControllerBase::~DisplayControllerBase() {
    if (state_ != ModuleState::UNINITIALIZED) {
        cleanup();
    }
    RADAR_INFO("DisplayControllerBase '{}' 销毁", moduleName_);
}

//==============================================================================
// IModule 接口实现
//==============================================================================

ErrorCode DisplayControllerBase::initialize() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_ != ModuleState::UNINITIALIZED) {
        RADAR_ERROR("DisplayController '{}' 已经初始化，当前状态: {}", moduleName_, static_cast<int>(state_));
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    RADAR_INFO("初始化 DisplayController '{}'...", moduleName_);

    try {
        // 调用派生类的初始化方法
        ErrorCode result = initializeDisplay();
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("DisplayController '{}' 初始化失败: 0x{:04X}", moduleName_, result);
            changeState(ModuleState::ERROR);
            return result;
        }

        // 设置状态为就绪
        changeState(ModuleState::READY);

        RADAR_INFO("DisplayController '{}' 初始化成功", moduleName_);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("DisplayController '{}' 初始化异常: {}", moduleName_, e.what());
        changeState(ModuleState::ERROR);
        return SystemErrors::INITIALIZATION_FAILED;
    }
}

ErrorCode DisplayControllerBase::start() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_ != ModuleState::READY && state_ != ModuleState::PAUSED) {
        RADAR_ERROR("DisplayController '{}' 无法启动，当前状态: {}", moduleName_, static_cast<int>(state_));
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    changeState(ModuleState::RUNNING);
    RADAR_INFO("DisplayController '{}' 已启动", moduleName_);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::stop() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_ != ModuleState::RUNNING && state_ != ModuleState::PAUSED) {
        RADAR_DEBUG("DisplayController '{}' 已停止或未运行", moduleName_);
        return SystemErrors::SUCCESS;
    }

    changeState(ModuleState::READY);
    RADAR_INFO("DisplayController '{}' 已停止", moduleName_);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::pause() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_ != ModuleState::RUNNING) {
        RADAR_ERROR("DisplayController '{}' 无法暂停，当前状态: {}", moduleName_, static_cast<int>(state_));
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    changeState(ModuleState::PAUSED);
    RADAR_INFO("DisplayController '{}' 已暂停", moduleName_);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::resume() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_ != ModuleState::PAUSED) {
        RADAR_ERROR("DisplayController '{}' 无法恢复，当前状态: {}", moduleName_, static_cast<int>(state_));
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    changeState(ModuleState::RUNNING);
    RADAR_INFO("DisplayController '{}' 已恢复", moduleName_);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::cleanup() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    RADAR_INFO("清理 DisplayController '{}'...", moduleName_);

    try {
        // 停止自动刷新
        autoRefreshEnabled_ = false;

        // 调用派生类的清理方法
        ErrorCode result = cleanupDisplay();
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("DisplayController '{}' 清理失败: 0x{:04X}", moduleName_, result);
        }

        // 重置状态
        changeState(ModuleState::UNINITIALIZED);

        RADAR_INFO("DisplayController '{}' 清理完成", moduleName_);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("DisplayController '{}' 清理异常: {}", moduleName_, e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ModuleState DisplayControllerBase::getState() const {
    return state_;
}

const std::string &DisplayControllerBase::getModuleName() const {
    return moduleName_;
}

void DisplayControllerBase::setStateChangeCallback(StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    stateChangeCallback_ = callback;
}

void DisplayControllerBase::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    errorCallback_ = callback;
}

PerformanceMetricsPtr DisplayControllerBase::getPerformanceMetrics() const {
    // 基类返回空指针，派生类可以实现具体的性能指标
    return nullptr;
}

//==============================================================================
// IDisplayController 接口实现
//==============================================================================

ErrorCode DisplayControllerBase::displayResult(const ProcessingResult &result,
                                               radar::IDisplayController::DisplayFormat format) {
    if (state_ != ModuleState::RUNNING) {
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    try {
        // 创建显示数据
        DisplayData displayData;
        displayData.sourceResult = result;
        displayData.format = format;
        displayData.displayTime = std::chrono::high_resolution_clock::now();
        displayData.sourcePacketId = result.sourcePacketId;

        // 调用派生类的渲染方法
        return renderData(displayData);
    } catch (const std::exception &e) {
        RADAR_ERROR("DisplayController '{}' 显示结果异常: {}", moduleName_, e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode DisplayControllerBase::displayMetrics(const SystemPerformanceMetrics &metrics,
                                                radar::IDisplayController::DisplayFormat format) {
    // 标记未使用的参数以避免编译警告
    (void)metrics;
    (void)format;

    if (state_ != ModuleState::RUNNING) {
        return DisplayControllerErrors::DISPLAY_NOT_READY;
    }

    // 基类提供默认实现
    RADAR_DEBUG("DisplayController '{}' 显示性能指标", moduleName_);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::setUpdateInterval(uint32_t intervalMs) {
    if (intervalMs < 10 || intervalMs > 10000) {
        return SystemErrors::INVALID_PARAMETER;
    }

    updateIntervalMs_ = intervalMs;
    RADAR_INFO("DisplayController '{}' 更新间隔设置为: {}ms", moduleName_, intervalMs);
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::setAutoRefresh(bool enabled) {
    autoRefreshEnabled_ = enabled;
    RADAR_INFO("DisplayController '{}' 自动刷新: {}", moduleName_, enabled ? "启用" : "禁用");
    return SystemErrors::SUCCESS;
}

ErrorCode DisplayControllerBase::saveToFile(const std::string &filename,
                                            radar::IDisplayController::DisplayFormat format) {
    if (filename.empty()) {
        return SystemErrors::INVALID_PARAMETER;
    }

    try {
        // 创建临时显示数据
        DisplayData tempData;
        tempData.displayTime = std::chrono::high_resolution_clock::now();
        tempData.metadata.title = "Display Controller Export - " + moduleName_;
        tempData.format = format;

        // 调用派生类的保存方法
        return saveDisplayToFile(filename, tempData);
    } catch (const std::exception &e) {
        RADAR_ERROR("DisplayController '{}' 保存文件异常: {}", moduleName_, e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode DisplayControllerBase::clearDisplay() {
    RADAR_DEBUG("DisplayController '{}' 清空显示", moduleName_);
    return SystemErrors::SUCCESS;
}

std::vector<radar::IDisplayController::DisplayFormat> DisplayControllerBase::getSupportedFormats() const {
    return getSpecificSupportedFormats();
}

//==============================================================================
// 私有方法实现
//==============================================================================

void DisplayControllerBase::changeState(ModuleState newState) {
    ModuleState oldState = state_;
    state_ = newState;

    // 调用状态变化回调
    if (stateChangeCallback_) {
        try {
            stateChangeCallback_(oldState, newState);
        } catch (const std::exception &e) {
            RADAR_ERROR("DisplayController '{}' 状态变化回调异常: {}", moduleName_, e.what());
        }
    }
}

}  // namespace modules
}  // namespace radar
