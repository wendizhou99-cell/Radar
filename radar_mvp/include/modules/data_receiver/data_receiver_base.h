/**
 * @file data_receiver_base.h
 * @brief 雷达数据接收模块基础接口定义
 *
 * 本文件定义了雷达数据接收系统的基础抽象接口和核心类型。
 * 提供数据接收的通用功能框架，具体的接收机制由派生类实现。
 * 遵循 RAII 原则和线程安全设计。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see IDataReceiver
 * @see DataReceiver
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "common/error_codes.h"
#include "common/interfaces.h"
#include "common/logger.h"
#include "common/types.h"

namespace radar {
namespace modules {
/**
 * @brief 抽象数据接收器基类
 *
 * 提供数据接收的通用功能实现，具体的接收机制由派生类实现。
 * 该类遵循 RAII 原则，自动管理资源和线程生命周期。
 * 实现了 IDataReceiver 接口的所有必需方法。
 *
 * @details
 * 使用流程：
 * 1. 创建具体的接收器实例
 * 2. 调用 configure() 配置接收器
 * 3. 调用 start() 启动接收器
 * 4. 使用 receivePacket() 或设置回调函数接收数据
 * 5. 调用 stop() 停止接收
 * 6. 对象析构时自动清理资源
 *
 * @note 该类的所有公共方法都是线程安全的
 * @warning 在配置完成之前不要启动接收，否则可能出现未定义行为
 */
class DataReceiver : public IDataReceiver {
  public:
    using DataCallback = std::function<void(RawDataPacketPtr)>;
    using ErrorCallback = std::function<void(ErrorCode, const std::string &)>;

  protected:
    std::thread receptionThread_;          ///< 数据接收线程
    std::atomic<bool> running_{false};     ///< 运行状态标志
    std::atomic<bool> shouldStop_{false};  ///< 停止请求标志

    DataCallback dataCallback_;    ///< 数据处理回调函数
    ErrorCallback errorCallback_;  ///< 错误处理回调函数

    mutable std::mutex statsMutex_;            ///< 统计信息互斥锁
    mutable std::mutex packetQueueMutex_;      ///< 数据包队列互斥锁
    std::condition_variable packetAvailable_;  ///< 数据包可用条件变量

    std::queue<RawDataPacketPtr> packetQueue_;  ///< 接收数据包队列

    std::shared_ptr<spdlog::logger> logger_;      ///< 日志记录器
    std::unique_ptr<DataReceiverConfig> config_;  ///< 配置参数

  public:
    /**
     * @brief 构造函数
     * @param logger 日志记录器实例
     */
    explicit DataReceiver(std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief 析构函数
     *
     * 自动停止接收并清理所有资源。遵循 RAII 原则。
     */
    virtual ~DataReceiver();

    // 禁用拷贝构造和赋值（遵循 RAII 和单一所有权原则）
    DataReceiver(const DataReceiver &) = delete;
    DataReceiver &operator=(const DataReceiver &) = delete;

    // 支持移动构造和赋值
    DataReceiver(DataReceiver &&other) noexcept;
    DataReceiver &operator=(DataReceiver &&other) noexcept;

    // IDataReceiver 接口实现
    /**
     * @brief 配置数据接收参数
     * @param config 数据接收配置
     * @return 操作结果错误码
     */
    ErrorCode configure(const DataReceiverConfig &config) override;

    /**
     * @brief 接收单个数据包（同步方式）
     * @param packet 输出参数，接收到的数据包
     * @param timeoutMs 超时时间（毫秒），0表示无限等待
     * @return 操作结果错误码
     */
    ErrorCode receivePacket(RawDataPacketPtr &packet, uint32_t timeoutMs = 0) override;

    /**
     * @brief 异步接收数据包
     * @return 数据包的future对象
     */
    std::future<RawDataPacketPtr> receivePacketAsync() override;

    /**
     * @brief 设置数据包接收回调函数
     * @param callback 数据包接收完成回调
     */
    void setPacketReceivedCallback(std::function<void(RawDataPacketPtr)> callback) override;

    /**
     * @brief 获取接收缓冲区状态
     * @return 缓冲区使用统计信息
     */
    BufferStatus getBufferStatus() const override;

    /**
     * @brief 刷新接收缓冲区
     * @return 操作结果错误码
     */
    ErrorCode flushBuffer() override;

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

  protected:
    /**
     * @brief 数据接收主循环（纯虚函数）
     *
     * 派生类必须实现该方法，提供具体的数据接收逻辑。
     * 该方法在独立线程中运行，直到 shouldStop_ 变为 true。
     *
     * @note 实现时需要定期检查 shouldStop_ 标志
     * @note 接收到数据后应调用 onDataReceived() 方法
     * @note 发生错误时应调用 onErrorOccurred() 方法
     */
    virtual void receptionLoop() = 0;

    /**
     * @brief 处理接收到的原始数据
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小（字节）
     *
     * @note 该方法会验证数据有效性，解析数据包，更新统计信息，并调用用户回调
     */
    void onDataReceived(const uint8_t *data, size_t size);

    /**
     * @brief 处理接收错误
     *
     * @param errorCode 错误代码
     * @param errorMessage 错误描述信息
     */
    void onErrorOccurred(ErrorCode errorCode, const std::string &errorMessage);

    /**
     * @brief 验证原始数据有效性
     *
     * @param data 数据缓冲区
     * @param size 数据大小
     * @return true 如果数据有效，false 否则
     *
     * @note 基类提供基本验证，派生类可以重写以提供特定验证逻辑
     */
    virtual bool validateRawData(const uint8_t *data, size_t size) const;

    /**
     * @brief 解析原始数据为 RawDataPacket
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小
     * @return 解析后的数据包智能指针，解析失败时返回 nullptr
     *
     * @note 派生类应重写该方法以支持特定的数据格式
     */
    virtual RawDataPacketPtr parseRawDataPacket(const uint8_t *data, size_t size) const;

    /**
     * @brief 将数据包加入接收队列
     *
     * @param packet 数据包智能指针
     * @note 该方法是线程安全的
     */
    void enqueuePacket(RawDataPacketPtr packet);

    /**
     * @brief 从接收队列取出数据包
     *
     * @param timeoutMs 超时时间（毫秒）
     * @return 数据包智能指针，超时或队列为空时返回 nullptr
     * @note 该方法是线程安全的，会阻塞直到有数据或超时
     */
    RawDataPacketPtr dequeuePacket(uint32_t timeoutMs);

  private:
    ModuleState currentState_{ModuleState::UNINITIALIZED};  ///< 当前模块状态
};

}  // namespace modules
}  // namespace radar
