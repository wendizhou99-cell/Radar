/**
 * @file display_controller_statistics.h
 * @brief 显示控制器统计信息和性能监控接口定义
 *
 * 本文件定义了显示控制器的统计信息收集、性能监控和分析功能。
 * 提供实时性能数据收集、历史数据分析和性能报告生成能力。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see DisplayStatistics
 * @see PerformanceMonitor
 * @see StatisticsManager
 */

#pragma once

#include "common/types.h"
#include "common/error_codes.h"
#include "common/logger.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <thread>

namespace radar
{
    namespace modules
    {
        //==============================================================================
        // 显示统计数据结构定义
        //==============================================================================

        /**
         * @brief 显示性能统计信息
         * @details 包含显示过程中的各种性能指标
         */
        struct DisplayStatistics
        {
            // 基础计数统计
            std::atomic<uint64_t> totalFramesDisplayed{0}; ///< 显示的总帧数
            std::atomic<uint64_t> totalFramesDropped{0};   ///< 丢弃的总帧数
            std::atomic<uint64_t> totalDataSize{0};        ///< 显示的总数据量（字节）
            std::atomic<uint64_t> totalErrors{0};          ///< 总错误次数

            // 时间统计
            std::atomic<double> averageFrameTime{0.0}; ///< 平均帧时间（毫秒）
            std::atomic<double> averageLatency{0.0};   ///< 平均显示延迟（毫秒）
            std::atomic<double> minLatency{0.0};       ///< 最小延迟（毫秒）
            std::atomic<double> maxLatency{0.0};       ///< 最大延迟（毫秒）

            // 帧率统计
            std::atomic<uint32_t> currentFrameRate{0}; ///< 当前帧率
            std::atomic<uint32_t> maxFrameRate{0};     ///< 最大帧率
            std::atomic<uint32_t> averageFrameRate{0}; ///< 平均帧率

            // 缓冲区统计
            std::atomic<uint32_t> bufferUsagePercent{0}; ///< 缓冲区使用率（百分比）
            std::atomic<uint32_t> maxBufferUsage{0};     ///< 最大缓冲区使用量
            std::atomic<uint32_t> bufferOverflows{0};    ///< 缓冲区溢出次数

            // 质量统计
            std::atomic<double> displayQualityScore{100.0}; ///< 显示质量评分（0-100）
            std::atomic<uint64_t> successfulDisplays{0};    ///< 成功显示次数
            std::atomic<uint64_t> failedDisplays{0};        ///< 失败显示次数

            // 时间戳
            Timestamp startTime;      ///< 统计开始时间
            Timestamp lastUpdateTime; ///< 最后更新时间

            /**
             * @brief 重置所有统计信息
             */
            void reset();

            /**
             * @brief 计算成功率
             * @return 成功率（0.0-1.0）
             */
            double getSuccessRate() const;

            /**
             * @brief 获取运行时间（秒）
             * @return 运行时间
             */
            double getUptimeSeconds() const;

            /**
             * @brief 生成统计报告
             * @return 统计报告字符串
             */
            std::string generateReport() const;
        };

        /**
         * @brief 历史性能数据点
         */
        struct PerformanceDataPoint
        {
            Timestamp timestamp;  ///< 时间戳
            uint32_t frameRate;   ///< 帧率
            double latency;       ///< 延迟（毫秒）
            uint32_t bufferUsage; ///< 缓冲区使用率
            uint32_t errorCount;  ///< 错误计数
            double qualityScore;  ///< 质量评分
        };

        //==============================================================================
        // 性能监控器类
        //==============================================================================

        /**
         * @brief 显示性能监控器
         * @details 实时监控显示性能并提供分析功能
         */
        class PerformanceMonitor
        {
        public:
            /**
             * @brief 构造函数
             * @param maxHistorySize 最大历史数据保存数量
             */
            explicit PerformanceMonitor(size_t maxHistorySize = 1000);

            /**
             * @brief 析构函数
             */
            ~PerformanceMonitor() = default;

            /**
             * @brief 记录帧显示事件
             * @param frameTime 帧时间（毫秒）
             * @param latency 显示延迟（毫秒）
             * @param bufferUsage 缓冲区使用率
             */
            void recordFrame(double frameTime, double latency, uint32_t bufferUsage);

            /**
             * @brief 记录错误事件
             * @param errorCode 错误码
             * @param description 错误描述
             */
            void recordError(ErrorCode errorCode, const std::string &description);

            /**
             * @brief 更新质量评分
             * @param qualityScore 质量评分（0-100）
             */
            void updateQualityScore(double qualityScore);

            /**
             * @brief 获取当前统计信息
             * @return 统计信息引用
             */
            const DisplayStatistics &getStatistics() const;

            /**
             * @brief 获取历史性能数据
             * @param maxPoints 最大返回数据点数量
             * @return 历史性能数据
             */
            std::vector<PerformanceDataPoint> getHistoryData(size_t maxPoints = 100) const;

            /**
             * @brief 分析性能趋势
             * @param timeRangeSeconds 分析时间范围（秒）
             * @return 性能趋势报告
             */
            std::string analyzePerformanceTrend(double timeRangeSeconds = 60.0) const;

            /**
             * @brief 检测性能异常
             * @return 异常报告
             */
            std::string detectPerformanceAnomalies() const;

            /**
             * @brief 重置监控数据
             */
            void reset();

        private:
            mutable std::mutex dataMutex_;                 ///< 数据访问互斥锁
            DisplayStatistics statistics_;                 ///< 统计数据
            std::deque<PerformanceDataPoint> historyData_; ///< 历史数据
            size_t maxHistorySize_;                        ///< 最大历史数据大小

            /**
             * @brief 更新统计数据
             */
            void updateStatistics();

            /**
             * @brief 计算移动平均值
             * @param values 数值序列
             * @param windowSize 窗口大小
             * @return 移动平均值
             */
            std::vector<double> calculateMovingAverage(
                const std::vector<double> &values, size_t windowSize) const;
        };

        //==============================================================================
        // 统计管理器类
        //==============================================================================

        /**
         * @brief 显示统计管理器
         * @details 管理多个显示控制器的统计信息，提供聚合分析
         */
        class StatisticsManager
        {
        public:
            /**
             * @brief 构造函数
             */
            StatisticsManager();

            /**
             * @brief 析构函数
             */
            ~StatisticsManager();

            /**
             * @brief 注册显示控制器
             * @param name 控制器名称
             * @param monitor 性能监控器指针
             * @return 操作结果错误码
             */
            ErrorCode registerController(const std::string &name,
                                         std::shared_ptr<PerformanceMonitor> monitor);

            /**
             * @brief 注销显示控制器
             * @param name 控制器名称
             * @return 操作结果错误码
             */
            ErrorCode unregisterController(const std::string &name);

            /**
             * @brief 获取指定控制器的统计信息
             * @param name 控制器名称
             * @return 统计信息指针
             */
            std::shared_ptr<const DisplayStatistics> getControllerStatistics(
                const std::string &name) const;

            /**
             * @brief 获取所有控制器的聚合统计信息
             * @return 聚合统计信息
             */
            DisplayStatistics getAggregatedStatistics() const;

            /**
             * @brief 生成系统级性能报告
             * @return 性能报告字符串
             */
            std::string generateSystemReport() const;

            /**
             * @brief 导出统计数据到文件
             * @param filePath 文件路径
             * @param format 导出格式（CSV、JSON等）
             * @return 操作结果错误码
             */
            ErrorCode exportStatistics(const std::string &filePath,
                                       const std::string &format = "CSV") const;

            /**
             * @brief 设置统计更新回调
             * @param callback 回调函数
             */
            void setStatisticsUpdateCallback(
                std::function<void(const std::string &, const DisplayStatistics &)> callback);

            /**
             * @brief 启动定期统计报告
             * @param intervalSeconds 报告间隔（秒）
             */
            void startPeriodicReporting(double intervalSeconds = 60.0);

            /**
             * @brief 停止定期统计报告
             */
            void stopPeriodicReporting();

        private:
            mutable std::mutex managerMutex_;                                                    ///< 管理器互斥锁
            std::map<std::string, std::shared_ptr<PerformanceMonitor>> monitors_;                ///< 监控器映射
            std::function<void(const std::string &, const DisplayStatistics &)> updateCallback_; ///< 更新回调

            // 定期报告线程
            std::atomic<bool> reportingActive_; ///< 报告线程活动标志
            std::thread reportingThread_;       ///< 报告线程
            double reportingInterval_;          ///< 报告间隔

            /**
             * @brief 定期报告线程函数
             */
            void reportingLoop();

            /**
             * @brief 生成CSV格式统计数据
             * @return CSV字符串
             */
            std::string generateCSVReport() const;

            /**
             * @brief 生成JSON格式统计数据
             * @return JSON字符串
             */
            std::string generateJSONReport() const;
        };

    } // namespace modules
} // namespace radar
