/**
 * @file base_receiver.cpp
 * @brief 雷达数据接收器基类实现
 *
 * 实现了数据接收器的基类DataReceiver，提供通用的接收流程、
 * 缓冲区管理、异步接收等功能。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "modules/data_receiver/data_receiver_base.h"
#include "common/logger.h"
#include "common/error_codes.h"

// 防止Windows宏定义与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>
#include <atomic>
#include <cstring>

namespace radar
{
    namespace modules
    {
        //==============================================================================
        // DataReceiver 构造函数和析构函数
        //==============================================================================

        DataReceiver::DataReceiver(std::shared_ptr<spdlog::logger> logger)
            : logger_(logger ? logger : spdlog::default_logger())
        {
            if (logger_)
            {
                logger_->info("DataReceiver created");
            }
        }

        DataReceiver::~DataReceiver()
        {
            // 确保停止接收线程
            if (running_.load())
            {
                stop();
            }

            // 等待线程结束
            if (receptionThread_.joinable())
            {
                receptionThread_.join();
            }

            if (logger_)
            {
                logger_->info("DataReceiver destroyed");
            }
        }

        //==============================================================================
        // 移动构造和赋值
        //==============================================================================

        DataReceiver::DataReceiver(DataReceiver &&other) noexcept
            : receptionThread_(std::move(other.receptionThread_)),
              running_(other.running_.load()),
              shouldStop_(other.shouldStop_.load()),
              dataCallback_(std::move(other.dataCallback_)),
              errorCallback_(std::move(other.errorCallback_)),
              packetQueue_(std::move(other.packetQueue_)),
              logger_(std::move(other.logger_)),
              config_(std::move(other.config_))
        {
            other.running_.store(false);
            other.shouldStop_.store(false);
        }

        DataReceiver &DataReceiver::operator=(DataReceiver &&other) noexcept
        {
            if (this != &other)
            {
                // 先停止当前对象
                if (running_.load())
                {
                    stop();
                }

                // 移动成员
                receptionThread_ = std::move(other.receptionThread_);
                running_.store(other.running_.load());
                shouldStop_.store(other.shouldStop_.load());
                dataCallback_ = std::move(other.dataCallback_);
                errorCallback_ = std::move(other.errorCallback_);
                packetQueue_ = std::move(other.packetQueue_);
                logger_ = std::move(other.logger_);
                config_ = std::move(other.config_);

                // 重置被移动对象的状态
                other.running_.store(false);
                other.shouldStop_.store(false);
            }
            return *this;
        }

        //==============================================================================
        // IDataReceiver 接口实现
        //==============================================================================

        ErrorCode DataReceiver::configure(const DataReceiverConfig &config)
        {
            std::lock_guard<std::mutex> lock(statsMutex_);

            try
            {
                config_ = std::make_unique<DataReceiverConfig>(config);
                if (logger_)
                {
                    logger_->info("DataReceiver configured successfully");
                }
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                if (logger_)
                {
                    logger_->error("Configuration failed: {}", e.what());
                }
                return SystemErrors::CONFIGURATION_ERROR;
            }
        }

        ErrorCode DataReceiver::receivePacket(RawDataPacketPtr &packet, uint32_t timeoutMs)
        {
            std::unique_lock<std::mutex> lock(packetQueueMutex_);

            if (timeoutMs == 0)
            {
                // 无限等待
                packetAvailable_.wait(lock, [this]
                                      { return !packetQueue_.empty() || shouldStop_.load(); });
            }
            else
            {
                // 有超时等待
                auto timeout = std::chrono::milliseconds(timeoutMs);
                if (!packetAvailable_.wait_for(lock, timeout, [this]
                                               { return !packetQueue_.empty() || shouldStop_.load(); }))
                {
                    return SystemErrors::OPERATION_TIMEOUT;
                }
            }

            if (shouldStop_.load())
            {
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            if (!packetQueue_.empty())
            {
                packet = packetQueue_.front();
                packetQueue_.pop();
                return SystemErrors::SUCCESS;
            }

            return DataReceiverErrors::RECEIVER_NOT_READY;
        }

        std::future<RawDataPacketPtr> DataReceiver::receivePacketAsync()
        {
            auto promise = std::make_shared<std::promise<RawDataPacketPtr>>();
            auto future = promise->get_future();

            std::thread([this, promise]()
                        {
                RawDataPacketPtr packet;
                ErrorCode result = receivePacket(packet, 0);

                if (result == SystemErrors::SUCCESS && packet) {
                    promise->set_value(packet);
                } else {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error("Failed to receive packet")));
                } })
                .detach();

            return future;
        }

        void DataReceiver::setPacketReceivedCallback(std::function<void(RawDataPacketPtr)> callback)
        {
            std::lock_guard<std::mutex> lock(packetQueueMutex_);
            dataCallback_ = callback;
            if (logger_)
            {
                logger_->info("Packet received callback set");
            }
        }

        BufferStatus DataReceiver::getBufferStatus() const
        {
            std::lock_guard<std::mutex> lock(packetQueueMutex_);

            BufferStatus status{};
            status.totalCapacity = 1000; // 默认容量
            status.currentSize = static_cast<uint32_t>(packetQueue_.size());
            status.peakSize = status.currentSize; // 简化实现
            status.totalReceived = 0;             // TODO: 实现统计
            status.totalDropped = 0;              // TODO: 实现统计

            return status;
        }

        ErrorCode DataReceiver::flushBuffer()
        {
            std::lock_guard<std::mutex> lock(packetQueueMutex_);

            size_t flushedCount = packetQueue_.size();
            std::queue<RawDataPacketPtr> empty;
            packetQueue_.swap(empty);

            if (logger_)
            {
                logger_->info("Buffer flushed - {} packets discarded", flushedCount);
            }

            return SystemErrors::SUCCESS;
        }

        //==============================================================================
        // IModule 接口实现
        //==============================================================================

        ErrorCode DataReceiver::initialize()
        {
            if (logger_)
            {
                logger_->info("DataReceiver initializing...");
            }
            return SystemErrors::SUCCESS;
        }

        ErrorCode DataReceiver::start()
        {
            // 检查是否已经运行
            if (running_.load())
            {
                if (logger_)
                {
                    logger_->warn("DataReceiver already running");
                }
                return DataReceiverErrors::RECEIVER_ALREADY_RUNNING;
            }

            // 检查是否已经初始化
            if (!config_)
            {
                if (logger_)
                {
                    logger_->error("DataReceiver not initialized");
                }
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            try
            {
                shouldStop_.store(false);
                running_.store(true);

                // 启动接收线程
                receptionThread_ = std::thread([this]()
                                               { receptionLoop(); });

                if (logger_)
                {
                    logger_->info("DataReceiver started successfully");
                }
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                running_.store(false);
                if (logger_)
                {
                    logger_->error("Failed to start DataReceiver: {}", e.what());
                }
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }
        }

        ErrorCode DataReceiver::stop()
        {
            if (!running_.load())
            {
                if (logger_)
                {
                    logger_->warn("DataReceiver not running");
                }
                return SystemErrors::SUCCESS;
            }

            try
            {
                shouldStop_.store(true);
                running_.store(false);

                // 通知所有等待的线程
                packetAvailable_.notify_all();

                // 等待接收线程结束
                if (receptionThread_.joinable())
                {
                    receptionThread_.join();
                }

                if (logger_)
                {
                    logger_->info("DataReceiver stopped successfully");
                }
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                if (logger_)
                {
                    logger_->error("Error during stop: {}", e.what());
                }
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }
        }

        ErrorCode DataReceiver::pause()
        {
            // 基础实现 - 子类可以重写
            if (logger_)
            {
                logger_->info("DataReceiver paused");
            }
            return SystemErrors::SUCCESS;
        }

        ErrorCode DataReceiver::resume()
        {
            // 基础实现 - 子类可以重写
            if (logger_)
            {
                logger_->info("DataReceiver resumed");
            }
            return SystemErrors::SUCCESS;
        }

        ErrorCode DataReceiver::cleanup()
        {
            if (running_.load())
            {
                stop();
            }

            if (logger_)
            {
                logger_->info("DataReceiver cleanup completed");
            }
            return SystemErrors::SUCCESS;
        }

        ModuleState DataReceiver::getState() const
        {
            if (running_.load())
            {
                return ModuleState::RUNNING;
            }
            else if (config_)
            {
                return ModuleState::READY;
            }
            else
            {
                return ModuleState::UNINITIALIZED;
            }
        }

        const std::string &DataReceiver::getModuleName() const
        {
            static const std::string name = "DataReceiver";
            return name;
        }

        void DataReceiver::setStateChangeCallback([[maybe_unused]] StateChangeCallback callback)
        {
            // TODO: 实现状态变化回调
            if (logger_)
            {
                logger_->info("State change callback set");
            }
        }

        void DataReceiver::setErrorCallback(ErrorCallback callback)
        {
            errorCallback_ = callback;
            if (logger_)
            {
                logger_->info("Error callback set");
            }
        }

        PerformanceMetricsPtr DataReceiver::getPerformanceMetrics() const
        {
            // TODO: 实现性能指标收集
            return nullptr;
        }

        //==============================================================================
        // 受保护的辅助方法
        //==============================================================================

        void DataReceiver::enqueuePacket(RawDataPacketPtr packet)
        {
            if (!packet)
                return;

            {
                std::lock_guard<std::mutex> lock(packetQueueMutex_);
                packetQueue_.push(packet);
            }

            // 通知等待的线程
            packetAvailable_.notify_one();

            // 调用用户回调
            if (dataCallback_)
            {
                try
                {
                    dataCallback_(packet);
                }
                catch (const std::exception &e)
                {
                    if (logger_)
                    {
                        logger_->error("Error in data callback: {}", e.what());
                    }
                }
            }
        }

        RawDataPacketPtr DataReceiver::dequeuePacket(uint32_t timeoutMs)
        {
            RawDataPacketPtr packet;
            receivePacket(packet, timeoutMs);
            return packet;
        }

        bool DataReceiver::validateRawData(const uint8_t *data, size_t size) const
        {
            return data != nullptr && size > 0;
        }

        RawDataPacketPtr DataReceiver::parseRawDataPacket(const uint8_t *data, size_t size) const
        {
            if (!validateRawData(data, size))
            {
                return nullptr;
            }

            // 基础实现 - 创建简单的数据包
            auto packet = std::make_shared<RawDataPacket>();
            packet->timestamp = std::chrono::high_resolution_clock::now();
            packet->iqData.resize(size / sizeof(ComplexFloat));

            // 简单复制数据（实际实现应该更复杂）
            std::memcpy(packet->iqData.data(), data, std::min(size, packet->iqData.size() * sizeof(ComplexFloat)));

            return packet;
        }

    } // namespace modules
} // namespace radar
