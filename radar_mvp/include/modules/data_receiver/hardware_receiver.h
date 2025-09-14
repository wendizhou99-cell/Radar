/**
 * @file hardware_receiver.h
 * @brief 硬件数据接收器头文件
 *
 * 定义了硬件数据接收器的类接口，支持真实雷达硬件设备的数据接收。
 * 提供硬件设备管理、数据接收和错误处理功能。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see HardwareReceiver
 */

#pragma once

#include "../data_receiver/data_receiver_base.h"
#include "common/types.h"
#include "common/error_codes.h"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>
#include <functional>

namespace radar
{
    namespace modules
    {
        // 前向声明
        class ThreadPool;

        /**
         * @brief 模拟目标结构
         */
        struct SimulatedTarget
        {
            double range;     ///< 目标距离（米）
            double velocity;  ///< 目标速度（米/秒）
            double rcs;       ///< 雷达截面积（平方米）
            double azimuth;   ///< 方位角（度）
            double elevation; ///< 俯仰角（度）
            double amplitude; ///< 信号幅度
        };

        /**
         * @brief 硬件数据接收器实现类
         *
         * 实现IDataReceiver接口，负责从硬件设备或模拟源接收雷达原始数据。
         * 支持真实硬件数据接收和模拟数据生成两种模式。
         */
        class HardwareReceiver : public IDataReceiver
        {
        public:
            /**
             * @brief 构造函数
             */
            HardwareReceiver();

            /**
             * @brief 析构函数
             */
            ~HardwareReceiver() override;

            // 禁用拷贝和赋值
            HardwareReceiver(const HardwareReceiver &) = delete;
            HardwareReceiver &operator=(const HardwareReceiver &) = delete;

            // IModule 接口实现
            ErrorCode initialize() override;
            ErrorCode start() override;
            ErrorCode stop() override;
            ErrorCode pause() override;
            ErrorCode resume() override;
            ErrorCode cleanup() override;
            ModuleState getState() const override { return state_; }
            const std::string &getModuleName() const override { return moduleName_; }
            void setStateChangeCallback(StateChangeCallback callback) override;
            void setErrorCallback(ErrorCallback callback) override;
            PerformanceMetricsPtr getPerformanceMetrics() const override;

            // IDataReceiver 接口实现
            ErrorCode configure(const DataReceiverConfig &config) override;
            ErrorCode receivePacket(RawDataPacketPtr &packet, uint32_t timeoutMs = 0) override;
            std::future<RawDataPacketPtr> receivePacketAsync() override;
            void setPacketReceivedCallback(std::function<void(RawDataPacketPtr)> callback) override;
            BufferStatus getBufferStatus() const override;
            ErrorCode flushBuffer() override;

        private:
            // 硬件管理方法
            ErrorCode initializeHardware();
            void stopHardware();
            void cleanupHardware();
            bool detectHardwareDevice();
            ErrorCode readHardwareData(RawDataPacketPtr &packet);

            // 模拟数据生成方法
            void initializeSimulation();
            RawDataPacketPtr generateSimulatedPacket();
            void generateChannelData(ComplexFloat *data, size_t samples, uint32_t channel);
            void addTargetEcho(ComplexFloat *data, size_t samples, uint32_t channel, const SimulatedTarget &target);
            void addClutter(ComplexFloat *data, size_t samples);

            // 缓冲区管理方法
            ErrorCode initializeBuffer();
            bool pushToBuffer(RawDataPacketPtr packet);

            // 线程函数
            void receiverThreadFunction();
            void monitorThreadFunction();

            // 性能监控方法
            void updatePerformanceMetrics();
            void logPerformanceMetrics();
            double calculateThroughput();

            // 错误处理方法
            void handleReceiveError(ErrorCode error);
            void attemptHardwareRecovery();
            void checkBufferHealth();

            // 状态管理
            void setState(ModuleState newState);

        private:
            // 性能监控
            struct PerformanceMonitor
            {
                std::chrono::high_resolution_clock::time_point startTime;
                std::chrono::high_resolution_clock::time_point lastReceiveTime;
                size_t peakBufferSize = 0;
                double packetsPerSecond = 0.0;
                double averageLatencyUs = 0.0;
                uint32_t errorCount = 0;
            };
            PerformanceMonitor performanceMonitor_;

            // 模拟数据生成
            std::mt19937 randomGenerator_;
            std::normal_distribution<float> noiseDistribution_;

            struct SimulationParams
            {
                double centerFrequency = 10e9;
                double samplingFrequency = 100e6;
                double pulseWidth = 1e-6;
                float noiseLevel = 0.1f;
                bool clutterEnabled = false;
            };
            SimulationParams simulationParams_;

            // 回调和同步
            std::function<void(RawDataPacketPtr)> packetReceivedCallback_;
            mutable std::mutex callbackMutex_;
            StateChangeCallback stateChangeCallback_;

            // 模块基本信息
            std::string moduleName_;
            std::atomic<ModuleState> state_;

            // 控制标志
            std::atomic<bool> isReceiving_;
            std::atomic<bool> shouldStop_;

            // 配置信息
            DataReceiverConfig config_;

            // 硬件相关
            std::unique_ptr<void, void (*)(void *)> hardwareDevice_;

            // 线程管理
            std::thread receiverThread_;
            std::thread monitorThread_;
            std::unique_ptr<ThreadPool> receiverThreadPool_;

            // 同步对象
            mutable std::mutex stateMutex_;
            mutable std::mutex bufferMutex_;
            mutable std::mutex configMutex_;
            std::condition_variable bufferNotEmpty_;
            std::condition_variable bufferNotFull_;

            // 数据缓冲区
            std::queue<RawDataPacketPtr> dataBuffer_;

            // 统计信息
            std::atomic<uint64_t> packetsReceived_;
            std::atomic<uint64_t> packetsDropped_;
            std::atomic<uint64_t> bytesReceived_;
            std::atomic<uint32_t> lastSequenceId_;

            // 模拟相关
            uint32_t simulationSeed_;
            std::vector<SimulatedTarget> simulatedTargets_;

            // 回调函数
            std::function<void(RawDataPacketPtr)> packetCallback_;
            StateChangeCallback stateCallback_;
            ErrorCallback errorCallback_;

            // 常量定义
            static constexpr uint32_t MAX_ERROR_COUNT = 10;
            static constexpr size_t MAX_PACKET_SIZE = 65536;
            static constexpr size_t MAX_QUEUE_SIZE = 1000;
            static constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;
        };

    } // namespace modules
} // namespace radar
