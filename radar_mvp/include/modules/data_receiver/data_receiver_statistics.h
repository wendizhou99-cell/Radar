/**
 * @file data_receiver_statistics.h
 * @brief 雷达数据接收统计信息管理模块
 *
 * 本文件定义了数据接收过程中的统计信息收集、分析和监控功能。
 * 提供线程安全的性能指标收集和实时监控能力。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see ReceptionStatistics
 * @see PerformanceMonitor
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>

#include "common/error_codes.h"
#include "common/types.h"

namespace radar {
namespace modules {
/**
 * @brief 数据接收统计信息结构
 *
 * 包含数据接收过程中的各种统计指标，用于性能监控和故障诊断。
 * 所有统计数据都是线程安全的原子操作。
 */
struct ReceptionStatistics {
    std::atomic<uint64_t> totalPacketsReceived{0};  ///< 接收的总数据包数
    std::atomic<uint64_t> totalBytesReceived{0};    ///< 接收的总字节数
    std::atomic<uint64_t> packetsDropped{0};        ///< 丢弃的数据包数
    std::atomic<uint64_t> invalidPackets{0};        ///< 无效数据包数
    std::atomic<double> averagePacketRate{0.0};     ///< 平均数据包接收率（包/秒）
    std::atomic<double> averageDataRate{0.0};       ///< 平均数据接收率（MB/秒）

    std::chrono::system_clock::time_point startTime_;       ///< 开始接收时间
    std::chrono::system_clock::time_point lastPacketTime_;  ///< 最后一个数据包时间

    /**
     * @brief 重置所有统计信息
     */
    void reset() {
        totalPacketsReceived = 0;
        totalBytesReceived = 0;
        packetsDropped = 0;
        invalidPackets = 0;
        averagePacketRate = 0.0;
        averageDataRate = 0.0;
        startTime_ = std::chrono::system_clock::now();
        lastPacketTime_ = startTime_;
    }

    /**
     * @brief 更新统计信息
     * @param packetSize 当前数据包大小（字节）
     */
    void updateStats(size_t packetSize) {
        totalPacketsReceived++;
        totalBytesReceived += packetSize;
        lastPacketTime_ = std::chrono::system_clock::now();

        // 计算速率（每秒更新一次）
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(lastPacketTime_ - startTime_).count();
        if (elapsed > 0) {
            averagePacketRate = static_cast<double>(totalPacketsReceived) / elapsed;
            averageDataRate = static_cast<double>(totalBytesReceived) / (elapsed * 1024 * 1024);
        }
    }

    /**
     * @brief 记录丢弃的数据包
     */
    void recordDroppedPacket() {
        packetsDropped++;
    }

    /**
     * @brief 记录无效数据包
     */
    void recordInvalidPacket() {
        invalidPackets++;
    }

    /**
     * @brief 获取接收成功率
     * @return 成功率百分比 (0.0-100.0)
     */
    double getSuccessRate() const {
        uint64_t total = totalPacketsReceived + packetsDropped;
        if (total == 0)
            return 100.0;
        return (static_cast<double>(totalPacketsReceived) / total) * 100.0;
    }

    /**
     * @brief 获取数据有效率
     * @return 有效率百分比 (0.0-100.0)
     */
    double getValidityRate() const {
        if (totalPacketsReceived == 0)
            return 100.0;
        uint64_t validPackets = totalPacketsReceived - invalidPackets;
        return (static_cast<double>(validPackets) / totalPacketsReceived) * 100.0;
    }

    /**
     * @brief 获取运行时长
     * @return 运行时长（秒）
     */
    double getRunningTimeSeconds() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration<double>(now - startTime_).count();
    }
};

/**
 * @brief 性能监控器类
 *
 * 提供高级的性能监控和分析功能，包括趋势分析、
 * 异常检测和性能预警等功能。
 */
class PerformanceMonitor {
  public:
    /**
     * @brief 构造函数
     * @param statistics 关联的统计信息结构
     */
    explicit PerformanceMonitor(ReceptionStatistics &statistics);

    /**
     * @brief 启动性能监控
     */
    void startMonitoring();

    /**
     * @brief 停止性能监控
     */
    void stopMonitoring();

    /**
     * @brief 检查是否存在性能异常
     * @return true 如果检测到异常，false 否则
     */
    bool hasPerformanceAnomaly() const;

    /**
     * @brief 获取性能警告信息
     * @return 警告信息列表
     */
    std::vector<std::string> getPerformanceWarnings() const;

    /**
     * @brief 生成性能报告
     * @return 格式化的性能报告字符串
     */
    std::string generatePerformanceReport() const;

  private:
    ReceptionStatistics &statistics_;
    mutable std::mutex monitorMutex_;
    std::atomic<bool> monitoring_{false};

    // 性能阈值配置
    static constexpr double MIN_SUCCESS_RATE = 95.0;    ///< 最低成功率阈值
    static constexpr double MIN_VALIDITY_RATE = 98.0;   ///< 最低有效率阈值
    static constexpr double MAX_PACKET_RATE = 10000.0;  ///< 最大包率阈值

    /**
     * @brief 检查成功率异常
     */
    bool checkSuccessRateAnomaly() const;

    /**
     * @brief 检查有效率异常
     */
    bool checkValidityRateAnomaly() const;

    /**
     * @brief 检查包率异常
     */
    bool checkPacketRateAnomaly() const;
};

/**
 * @brief 统计信息管理器
 *
 * 提供统计信息的持久化、历史数据管理和趋势分析功能。
 */
class StatisticsManager {
  public:
    /**
     * @brief 构造函数
     */
    StatisticsManager();

    /**
     * @brief 注册统计信息实例
     * @param name 统计实例名称
     * @param statistics 统计信息引用
     */
    void registerStatistics(const std::string &name, ReceptionStatistics &statistics);

    /**
     * @brief 注销统计信息实例
     * @param name 统计实例名称
     */
    void unregisterStatistics(const std::string &name);

    /**
     * @brief 保存统计信息到文件
     * @param filename 保存文件名
     * @return 操作结果错误码
     */
    ErrorCode saveStatisticsToFile(const std::string &filename) const;

    /**
     * @brief 从文件加载统计信息
     * @param filename 文件名
     * @return 操作结果错误码
     */
    ErrorCode loadStatisticsFromFile(const std::string &filename);

    /**
     * @brief 生成汇总报告
     * @return 汇总报告字符串
     */
    std::string generateSummaryReport() const;

  private:
    std::map<std::string, std::reference_wrapper<ReceptionStatistics>> statisticsMap_;
    mutable std::mutex managerMutex_;
};

}  // namespace modules
}  // namespace radar
