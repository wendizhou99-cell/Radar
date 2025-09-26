/**
 * @file data_receiver_implementations.h
 * @brief 雷达数据接收器具体实现类接口定义
 *
 * 本文件定义了各种具体的数据接收器实现类，包括：
 * - UDP 网络数据接收器
 * - 文件数据接收器
 * - 硬件数据接收器
 * - 模拟数据接收器
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see UDPDataReceiver
 * @see FileDataReceiver
 * @see HardwareDataReceiver
 * @see SimulationDataReceiver
 */

#pragma once

#include <fstream>

#include "data_receiver_base.h"
#include "data_receiver_statistics.h"

namespace radar {
namespace modules {
/**
 * @brief UDP 网络数据接收器
 *
 * 基于 UDP 协议的高性能网络数据接收器，适用于实时雷达数据传输。
 * 支持高吞吐量数据接收和网络错误自动恢复。
 *
 * @details
 * 特性：
 * - 异步 I/O 操作，最小化延迟
 * - 自动网络错误检测和恢复
 * - 可配置的接收缓冲区大小
 * - 支持多播和广播接收
 * - 内置数据包完整性检查
 *
 * @note Windows 和 Linux 平台都支持
 * @warning 大数据量传输时注意系统网络缓冲区配置
 */
class UDPDataReceiver : public DataReceiver {
  private:
    int socketFd_{-1};               ///< UDP 套接字文件描述符
    bool socketInitialized_{false};  ///< 套接字初始化状态

  public:
    /**
     * @brief 构造函数
     * @param logger 日志记录器实例
     */
    explicit UDPDataReceiver(std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief 析构函数
     * 自动关闭套接字和清理资源
     */
    ~UDPDataReceiver() override;

  protected:
    /**
     * @brief UDP 数据接收主循环
     *
     * 在独立线程中运行，持续接收 UDP 数据包直到收到停止信号。
     */
    void receptionLoop() override;

    /**
     * @brief 解析 UDP 数据包
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小
     * @return 解析后的雷达数据包，失败时返回 nullptr
     */
    RawDataPacketPtr parseRawDataPacket(const uint8_t *data, size_t size) const override;

  private:
    /**
     * @brief 设置 UDP 套接字
     * @return true 如果设置成功，false 否则
     */
    bool setupSocket();

    /**
     * @brief 关闭 UDP 套接字
     */
    void closeSocket();

    /**
     * @brief 配置套接字选项
     * @return true 如果配置成功，false 否则
     */
    bool configureSocket();
};

/**
 * @brief 文件数据接收器
 *
 * 从文件读取数据并模拟实时数据接收，主要用于测试和回放场景。
 * 支持多种文件格式和播放模式。
 *
 * @details
 * 特性：
 * - 支持二进制和文本文件格式
 * - 可配置的播放速率控制
 * - 循环播放支持
 * - 大文件分块读取
 * - 播放进度监控
 *
 * @note 适用于离线测试和算法验证
 * @warning 大文件读取时注意内存使用量
 */
class FileDataReceiver : public DataReceiver {
  private:
    std::ifstream dataFile_;         ///< 数据文件流
    size_t currentFilePosition_{0};  ///< 当前文件读取位置
    size_t totalFileSize_{0};        ///< 文件总大小
    bool fileOpened_{false};         ///< 文件打开状态

  public:
    /**
     * @brief 构造函数
     * @param logger 日志记录器实例
     */
    explicit FileDataReceiver(std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief 析构函数
     * 自动关闭文件和清理资源
     */
    ~FileDataReceiver() override;

    /**
     * @brief 获取播放进度
     * @return 播放进度百分比 (0.0 - 1.0)
     */
    double getPlaybackProgress() const;

    /**
     * @brief 跳转到指定位置
     * @param position 目标位置（字节偏移）
     * @return true 如果跳转成功，false 否则
     */
    bool seekToPosition(size_t position);

  protected:
    /**
     * @brief 文件数据读取主循环
     *
     * 按配置的时间间隔从文件读取数据，模拟实时数据流。
     */
    void receptionLoop() override;

    /**
     * @brief 解析文件数据包
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小
     * @return 解析后的雷达数据包，失败时返回 nullptr
     */
    RawDataPacketPtr parseRawDataPacket(const uint8_t *data, size_t size) const override;

  private:
    /**
     * @brief 打开数据文件
     * @return true 如果打开成功，false 否则
     */
    bool openFile();

    /**
     * @brief 关闭数据文件
     */
    void closeFile();

    /**
     * @brief 获取文件大小
     * @return 文件大小（字节），失败时返回 0
     */
    size_t getFileSize() const;

    /**
     * @brief 重置文件读取位置到开头
     * @return true 如果重置成功，false 否则
     */
    bool resetFilePosition();
};

/**
 * @brief 硬件数据接收器
 *
 * 从真实雷达硬件设备接收数据的接收器实现。
 * 支持多种硬件接口和协议。
 *
 * @details
 * 特性：
 * - 支持多种硬件接口（PCIe、USB、以太网等）
 * - 硬件状态监控和错误恢复
 * - 高速数据传输优化
 * - 设备热插拔支持
 * - 硬件时钟同步
 *
 * @note 需要相应的硬件驱动支持
 * @warning 硬件故障时会自动切换到模拟模式
 */
class HardwareDataReceiver : public DataReceiver {
  private:
    void *hardwareHandle_{nullptr};    ///< 硬件设备句柄
    bool hardwareInitialized_{false};  ///< 硬件初始化状态
    std::string deviceIdentifier_;     ///< 设备标识符

  public:
    /**
     * @brief 构造函数
     * @param logger 日志记录器实例
     */
    explicit HardwareDataReceiver(std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief 析构函数
     * 自动关闭硬件设备和清理资源
     */
    ~HardwareDataReceiver() override;

    /**
     * @brief 检测可用的硬件设备
     * @return 设备列表
     */
    std::vector<std::string> detectAvailableDevices() const;

    /**
     * @brief 获取硬件设备信息
     * @return 设备信息字符串
     */
    std::string getDeviceInfo() const;

    /**
     * @brief 检查硬件状态
     * @return true 如果硬件正常，false 否则
     */
    bool isHardwareHealthy() const;

  protected:
    /**
     * @brief 硬件数据接收主循环
     *
     * 从硬件设备持续读取数据直到收到停止信号。
     */
    void receptionLoop() override;

    /**
     * @brief 解析硬件数据包
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小
     * @return 解析后的雷达数据包，失败时返回 nullptr
     */
    RawDataPacketPtr parseRawDataPacket(const uint8_t *data, size_t size) const override;

  private:
    /**
     * @brief 初始化硬件设备
     * @return true 如果初始化成功，false 否则
     */
    bool initializeHardware();

    /**
     * @brief 关闭硬件设备
     */
    void closeHardware();

    /**
     * @brief 配置硬件参数
     * @return true 如果配置成功，false 否则
     */
    bool configureHardware();

    /**
     * @brief 重置硬件设备
     * @return true 如果重置成功，false 否则
     */
    bool resetHardware();
};

/**
 * @brief 模拟数据接收器
 *
 * 生成符合雷达特征的模拟数据，用于测试和开发。
 * 可以模拟各种雷达场景和目标特征。
 *
 * @details
 * 特性：
 * - 可配置的目标模拟
 * - 真实的噪声和杂波模型
 * - 多种雷达工作模式模拟
 * - 环境因素影响模拟
 * - 数据重现性保证
 *
 * @note 主要用于算法开发和测试
 * @warning 模拟数据不能完全替代真实数据测试
 */
class SimulationDataReceiver : public DataReceiver {
  private:
    uint32_t simulationSeed_{42};    ///< 随机数种子
    bool deterministicMode_{false};  ///< 确定性模式标志

    struct SimulationTarget {
        double range;      ///< 目标距离（米）
        double velocity;   ///< 目标速度（米/秒）
        double rcs;        ///< 雷达截面积（平方米）
        double azimuth;    ///< 方位角（度）
        double elevation;  ///< 俯仰角（度）
    };

    std::vector<SimulationTarget> targets_;  ///< 模拟目标列表

  public:
    /**
     * @brief 构造函数
     * @param logger 日志记录器实例
     */
    explicit SimulationDataReceiver(std::shared_ptr<spdlog::logger> logger = nullptr);

    /**
     * @brief 析构函数
     */
    ~SimulationDataReceiver() override = default;

    /**
     * @brief 添加模拟目标
     * @param range 目标距离
     * @param velocity 目标速度
     * @param rcs 雷达截面积
     * @param azimuth 方位角
     * @param elevation 俯仰角
     */
    void addSimulationTarget(double range, double velocity, double rcs, double azimuth, double elevation);

    /**
     * @brief 清除所有模拟目标
     */
    void clearSimulationTargets();

    /**
     * @brief 设置确定性模式
     * @param deterministic true 启用确定性模式，false 随机模式
     */
    void setDeterministicMode(bool deterministic);

    /**
     * @brief 设置随机数种子
     * @param seed 随机数种子
     */
    void setSimulationSeed(uint32_t seed);

  protected:
    /**
     * @brief 模拟数据生成主循环
     *
     * 按照配置的参数生成模拟雷达数据。
     */
    void receptionLoop() override;

    /**
     * @brief 解析模拟数据包
     *
     * @param data 原始数据缓冲区
     * @param size 数据大小
     * @return 解析后的雷达数据包，失败时返回 nullptr
     */
    RawDataPacketPtr parseRawDataPacket(const uint8_t *data, size_t size) const override;

  private:
    /**
     * @brief 生成单个模拟数据包
     * @return 模拟数据包
     */
    RawDataPacketPtr generateSimulatedPacket();

    /**
     * @brief 生成目标回波信号
     * @param target 目标参数
     * @return 目标信号数据
     */
    std::vector<ComplexFloat> generateTargetEcho(const SimulationTarget &target);

    /**
     * @brief 生成噪声信号
     * @param samples 采样点数
     * @return 噪声信号数据
     */
    std::vector<ComplexFloat> generateNoise(size_t samples);

    /**
     * @brief 生成杂波信号
     * @param samples 采样点数
     * @return 杂波信号数据
     */
    std::vector<ComplexFloat> generateClutter(size_t samples);
};

}  // namespace modules
}  // namespace radar
