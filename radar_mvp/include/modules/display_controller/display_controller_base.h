/**
 * @file display_controller_base.h
 * @brief 雷达显示控制模块基础接口定义
 *
 * 本文件定义了雷达显示控制系统的基础抽象接口和核心类型。
 * 提供显示控制的通用功能框架，具体的显示机制由派生类实现。
 * 遵循 RAII 原则和线程安全设计。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see IDisplayController
 * @see DisplayControllerBase
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "../../common/error_codes.h"
#include "../../common/interfaces.h"
#include "../../common/logger.h"
#include "../../common/types.h"

namespace radar {
namespace modules {
//==============================================================================
// 显示控制相关类型定义
//==============================================================================

/**
 * @brief 显示数据结构
 * @details 封装要显示的处理结果数据，包含格式化和时间戳信息
 */
struct DisplayData {
    Timestamp displayTime;                            ///< 显示时间戳
    uint64_t sourcePacketId;                          ///< 源数据包ID
    radar::IDisplayController::DisplayFormat format;  ///< 显示格式
    std::string formattedData;                        ///< 格式化后的显示数据
    ProcessingResult sourceResult;                    ///< 原始处理结果

    /// 显示元数据
    struct Metadata {
        std::string title;                 ///< 显示标题
        std::vector<std::string> headers;  ///< 表格头部（如果适用）
        uint32_t priority;                 ///< 显示优先级
        bool requiresRealTime;             ///< 是否需要实时显示
    } metadata;

    /**
     * @brief 检查显示数据的有效性
     * @return 数据是否有效
     */
    bool isValid() const {
        return !formattedData.empty() && sourcePacketId > 0;
    }

    /**
     * @brief 获取数据大小（字节）
     * @return 数据大小
     */
    size_t getDataSize() const {
        return formattedData.size() + sizeof(DisplayData);
    }
};

/**
 * @brief 显示器状态信息
 */
struct DisplayStatus {
    uint32_t totalFramesDisplayed;  ///< 显示的总帧数
    uint32_t totalFramesDropped;    ///< 丢弃的总帧数
    uint32_t currentFrameRate;      ///< 当前帧率
    uint32_t bufferUsage;           ///< 缓冲区使用率（百分比）
    double averageLatency;          ///< 平均显示延迟（毫秒）
    Timestamp lastUpdateTime;       ///< 最后更新时间
};

//==============================================================================
// 显示控制器基类实现
//==============================================================================

/**
 * @brief 抽象显示控制器基类
 *
 * 提供显示控制的通用功能实现，具体的显示机制由派生类实现。
 * 该类遵循 RAII 原则，自动管理资源和线程生命周期。
 * 实现了 IDisplayController 接口的大部分通用方法。
 *
 * @details
 * 使用流程：
 * 1. 创建具体的显示控制器实例
 * 2. 调用 initialize() 初始化资源
 * 3. 调用 start() 启动显示线程
 * 4. 使用 displayResult() 显示数据
 * 5. 调用 stop() 和 cleanup() 清理资源
 */
class DisplayControllerBase : public radar::IDisplayController {
  public:
    /**
     * @brief 构造函数
     * @param name 显示控制器名称
     */
    explicit DisplayControllerBase(const std::string &name);

    /**
     * @brief 虚析构函数
     */
    virtual ~DisplayControllerBase();

    // 禁用拷贝构造和赋值
    DisplayControllerBase(const DisplayControllerBase &) = delete;
    DisplayControllerBase &operator=(const DisplayControllerBase &) = delete;

    // 允许移动构造和赋值
    DisplayControllerBase(DisplayControllerBase &&) = default;
    DisplayControllerBase &operator=(DisplayControllerBase &&) = default;

    //==============================================================================
    // IModule 接口实现
    //==============================================================================

    ErrorCode initialize() override;
    ErrorCode start() override;
    ErrorCode stop() override;
    ErrorCode pause() override;
    ErrorCode resume() override;
    ErrorCode cleanup() override;

    ModuleState getState() const override;
    const std::string &getModuleName() const override;
    void setStateChangeCallback(StateChangeCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;
    PerformanceMetricsPtr getPerformanceMetrics() const override;

    //==============================================================================
    // IDisplayController 接口实现
    //==============================================================================

    ErrorCode displayResult(const ProcessingResult &result,
                            radar::IDisplayController::DisplayFormat format =
                                radar::IDisplayController::DisplayFormat::CONSOLE_TEXT) override;

    ErrorCode displayMetrics(const SystemPerformanceMetrics &metrics,
                             radar::IDisplayController::DisplayFormat format =
                                 radar::IDisplayController::DisplayFormat::CONSOLE_TEXT) override;

    ErrorCode setUpdateInterval(uint32_t intervalMs) override;

    ErrorCode setAutoRefresh(bool enabled) override;

    ErrorCode saveToFile(const std::string &filename, radar::IDisplayController::DisplayFormat format =
                                                          radar::IDisplayController::DisplayFormat::FILE_CSV) override;

    ErrorCode clearDisplay() override;

    std::vector<radar::IDisplayController::DisplayFormat> getSupportedFormats() const override;

  protected:
    //==============================================================================
    // 受保护的虚函数接口（由派生类实现）
    //==============================================================================

    /**
     * @brief 初始化具体的显示资源
     * @return 操作结果错误码
     */
    virtual ErrorCode initializeDisplay() = 0;

    /**
     * @brief 清理具体的显示资源
     * @return 操作结果错误码
     */
    virtual ErrorCode cleanupDisplay() = 0;

    /**
     * @brief 渲染显示数据
     * @param data 要渲染的显示数据
     * @return 操作结果错误码
     */
    virtual ErrorCode renderData(const DisplayData &data) = 0;

    /**
     * @brief 获取支持的显示格式
     * @return 支持的格式列表
     */
    virtual std::vector<radar::IDisplayController::DisplayFormat> getSpecificSupportedFormats() const = 0;

    /**
     * @brief 保存显示内容到文件的具体实现
     * @param filePath 文件路径
     * @param data 显示数据
     * @return 操作结果错误码
     */
    virtual ErrorCode saveDisplayToFile(const std::string &filePath, const DisplayData &data) = 0;

    //==============================================================================
    // 受保护的工具方法
    //==============================================================================

    /**
     * @brief 创建显示数据
     * @param result 处理结果
     * @param format 显示格式
     * @return 显示数据
     */
    DisplayData createDisplayData(const ProcessingResult &result, radar::IDisplayController::DisplayFormat format);

    /**
     * @brief 格式化处理结果
     * @param result 处理结果
     * @param format 目标格式
     * @return 格式化后的字符串
     */
    std::string formatProcessingResult(const ProcessingResult &result, radar::IDisplayController::DisplayFormat format);

    /**
     * @brief 更新性能统计
     */
    void updateStatistics();

    /**
     * @brief 控制帧率
     */
    void controlFrameRate();

    //==============================================================================
    // 受保护的成员变量
    //==============================================================================

    mutable std::mutex stateMutex_;   ///< 状态访问互斥锁
    mutable std::mutex bufferMutex_;  ///< 缓冲区访问互斥锁
    mutable std::mutex configMutex_;  ///< 配置访问互斥锁

    ModuleState state_;       ///< 模块状态
    std::string moduleName_;  ///< 模块名称

    // 线程管理
    std::atomic<bool> running_;              ///< 运行状态标志
    std::atomic<bool> shouldStop_;           ///< 停止信号标志
    std::thread displayThread_;              ///< 显示线程
    std::condition_variable dataAvailable_;  ///< 数据可用条件变量

    // 数据缓冲区
    std::deque<DisplayData> displayBuffer_;  ///< 显示数据缓冲区

    // 性能统计
    std::atomic<uint64_t> totalFramesDisplayed_;                    ///< 显示的总帧数
    std::atomic<uint64_t> totalFramesDropped_;                      ///< 丢弃的总帧数
    std::atomic<uint32_t> currentFrameRate_;                        ///< 当前帧率
    std::chrono::high_resolution_clock::time_point lastFrameTime_;  ///< 上一帧时间

    // 显示配置
    std::atomic<uint32_t> updateIntervalMs_;  ///< 更新间隔（毫秒）
    std::atomic<bool> autoRefreshEnabled_;    ///< 自动刷新启用标志

    // 回调函数
    StateChangeCallback stateChangeCallback_;  ///< 状态变化回调
    ErrorCallback errorCallback_;              ///< 错误回调

  private:
    /**
     * @brief 显示线程主循环
     */
    void displayLoop();

    /**
     * @brief 改变模块状态
     * @param newState 新状态
     */
    void changeState(ModuleState newState);

    /**
     * @brief 初始化缓冲区
     * @return 操作结果错误码
     */
    ErrorCode initializeBuffer();

    /**
     * @brief 清理缓冲区
     */
    void cleanupBuffer();
};

}  // namespace modules
}  // namespace radar
