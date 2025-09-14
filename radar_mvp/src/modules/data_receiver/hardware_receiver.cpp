/**
 * @file hardware_receiver.cpp
 * @brief 硬件数据接收器实现
 *
 * 实现了IDataReceiver接口，负责从硬件设备或模拟源接收雷达原始数据。
 * 支持真实硬件数据接收和模拟数据生成两种模式，提供高性能的缓冲区管理
 * 和异步数据接收功能。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-01-14
 * @since 1.0
 *
 * @see IDataReceiver
 * @see DataReceiverConfig
 */

#include "modules/data_receiver/hardware_receiver.h"
#include "modules/data_receiver/data_receiver_implementations.h"
#include "common/logger.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

namespace radar::modules
{
    //==============================================================================
    // HardwareDataReceiver 实现
    //==============================================================================

    HardwareDataReceiver::HardwareDataReceiver(std::shared_ptr<spdlog::logger> logger)
        : DataReceiver(logger)
    {
        if (logger_)
        {
            logger_->info("HardwareDataReceiver created");
        }
    }

    HardwareDataReceiver::~HardwareDataReceiver()
    {
        if (logger_)
        {
            logger_->info("HardwareDataReceiver destroyed");
        }
    }

    void HardwareDataReceiver::receptionLoop()
    {
        if (logger_)
        {
            logger_->info("Hardware reception loop started");
        }

        // 简化的接收循环实现
        while (!shouldStop_.load())
        {
            // TODO: 实现实际的硬件接收逻辑
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 模拟接收数据包
            const size_t dataSize = 1024;
            auto buffer = std::make_unique<uint8_t[]>(dataSize);

            // 模拟一些数据
            for (size_t i = 0; i < dataSize; ++i)
            {
                buffer[i] = static_cast<uint8_t>(i % 256);
            }

            // 解析并入队
            auto packet = parseRawDataPacket(buffer.get(), dataSize);
            if (packet)
            {
                enqueuePacket(packet);
            }
        }

        if (logger_)
        {
            logger_->info("Hardware reception loop stopped");
        }
    }

    std::vector<std::string> HardwareDataReceiver::detectAvailableDevices() const
    {
        return {"mock_device_0", "mock_device_1"};
    }

    std::string HardwareDataReceiver::getDeviceInfo() const
    {
        return "Mock Hardware Device v1.0";
    }

    bool HardwareDataReceiver::isHardwareHealthy() const
    {
        return true;
    }

    RawDataPacketPtr HardwareDataReceiver::parseRawDataPacket(const uint8_t *data, size_t size) const
    {
        // 调用基类的默认实现
        return DataReceiver::parseRawDataPacket(data, size);
    }

    // 简单的ThreadPool实现
    class ThreadPool
    {
    public:
        explicit ThreadPool(int numThreads) : stop(false)
        {
            for (int i = 0; i < numThreads; ++i)
            {
                workers.emplace_back([this]
                                     {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    } });
            }
        }

        template <class F, class... Args>
        auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
        {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (stop)
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([task]()
                              { (*task)(); });
            }
            condition.notify_one();
            return res;
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (std::thread &worker : workers)
                worker.join();
        }

    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };
}

// 防止Windows宏定义与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif

#include <random>
#include <cmath>
#include <numeric>
#include <thread>
#include <chrono>

namespace radar
{
    namespace modules
    {
        using radar::ModuleState;

        //==============================================================================
        // 构造函数和析构函数
        //==============================================================================

        HardwareReceiver::HardwareReceiver()
            : randomGenerator_(42),
              noiseDistribution_(0.0f, 1.0f),
              moduleName_("HardwareReceiver"),
              state_(ModuleState::UNINITIALIZED),
              isReceiving_(false),
              shouldStop_(false),
              hardwareDevice_(nullptr, [](void *) {}),
              packetsReceived_(0),
              packetsDropped_(0),
              bytesReceived_(0),
              lastSequenceId_(0),
              simulationSeed_(42)
        {
            // 初始化随机数生成器
            randomGenerator_.seed(simulationSeed_);
            noiseDistribution_ = std::normal_distribution<float>(0.0f, 1.0f);

            RADAR_DEBUG("HardwareReceiver constructor called");
        }

        HardwareReceiver::~HardwareReceiver()
        {
            RADAR_DEBUG("HardwareReceiver destructor called");

            // 确保清理资源
            if (state_ != ModuleState::UNINITIALIZED)
            {
                cleanup();
            }
        }

        //==============================================================================
        // IModule 接口实现
        //==============================================================================

        ErrorCode HardwareReceiver::initialize()
        {
            std::lock_guard<std::mutex> lock(stateMutex_);

            MODULE_INFO(DataReceiver, "Initializing HardwareReceiver");

            // 检查当前状态
            if (state_ != ModuleState::UNINITIALIZED)
            {
                MODULE_WARN(DataReceiver, "HardwareReceiver already initialized");
                return SystemErrors::SUCCESS;
            }

            // 状态转换
            setState(ModuleState::INITIALIZING);

            try
            {
                // 初始化接收缓冲区
                ErrorCode result = initializeBuffer();
                if (!isSuccess(result))
                {
                    MODULE_ERROR(DataReceiver, "Failed to initialize buffer: 0x{:X}", result);
                    setState(ModuleState::ERROR);
                    return result;
                }

                // 初始化硬件设备（如果不是模拟模式）
                if (!config_.simulationEnabled)
                {
                    result = initializeHardware();
                    if (!isSuccess(result))
                    {
                        MODULE_ERROR(DataReceiver, "Failed to initialize hardware: 0x{:X}", result);
                        setState(ModuleState::ERROR);
                        return result;
                    }
                }
                else
                {
                    // 初始化模拟数据生成器
                    initializeSimulation();
                }

                // 创建接收线程池
                receiverThreadPool_ = std::make_unique<ThreadPool>(
                    config_.simulationEnabled ? 1 : 2 // 模拟模式1个线程，硬件模式2个线程
                );

                // 初始化性能监控
                performanceMonitor_ = PerformanceMonitor{}; // 重置性能监控
                performanceMonitor_.startTime = std::chrono::high_resolution_clock::now();

                setState(ModuleState::READY);
                MODULE_INFO(DataReceiver, "HardwareReceiver initialized successfully");

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                MODULE_ERROR(DataReceiver, "Exception during initialization: {}", e.what());
                setState(ModuleState::ERROR);
                return SystemErrors::INITIALIZATION_FAILED;
            }
        }

        ErrorCode HardwareReceiver::start()
        {
            std::lock_guard<std::mutex> lock(stateMutex_);

            MODULE_INFO(DataReceiver, "Starting HardwareReceiver");

            // 检查状态
            if (state_ != ModuleState::READY && state_ != ModuleState::PAUSED)
            {
                MODULE_ERROR(DataReceiver, "Cannot start from state: {}",
                             static_cast<int>(state_.load()));
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            // 重置控制标志
            shouldStop_ = false;
            isReceiving_ = true;

            // 启动接收线程
            receiverThread_ = std::thread(&HardwareReceiver::receiverThreadFunction, this);

            // 启动监控线程（如果需要）
            if (config_.simulationEnabled)
            {
                monitorThread_ = std::thread(&HardwareReceiver::monitorThreadFunction, this);
            }

            setState(ModuleState::RUNNING);
            MODULE_INFO(DataReceiver, "HardwareReceiver started successfully");

            return SystemErrors::SUCCESS;
        }

        ErrorCode HardwareReceiver::stop()
        {
            MODULE_INFO(DataReceiver, "Stopping HardwareReceiver");

            // 设置停止标志
            shouldStop_ = true;
            isReceiving_ = false;

            // 通知所有等待的线程
            bufferNotEmpty_.notify_all();
            bufferNotFull_.notify_all();

            // 等待线程结束
            if (receiverThread_.joinable())
            {
                receiverThread_.join();
            }

            if (monitorThread_.joinable())
            {
                monitorThread_.join();
            }

            // 停止硬件设备
            if (!config_.simulationEnabled && hardwareDevice_)
            {
                stopHardware();
            }

            std::lock_guard<std::mutex> lock(stateMutex_);
            setState(ModuleState::READY);

            MODULE_INFO(DataReceiver, "HardwareReceiver stopped");

            return SystemErrors::SUCCESS;
        }

        ErrorCode HardwareReceiver::pause()
        {
            std::lock_guard<std::mutex> lock(stateMutex_);

            if (state_ != ModuleState::RUNNING)
            {
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            isReceiving_ = false;
            setState(ModuleState::PAUSED);

            MODULE_INFO(DataReceiver, "HardwareReceiver paused");

            return SystemErrors::SUCCESS;
        }

        ErrorCode HardwareReceiver::resume()
        {
            std::lock_guard<std::mutex> lock(stateMutex_);

            if (state_ != ModuleState::PAUSED)
            {
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            isReceiving_ = true;
            setState(ModuleState::RUNNING);
            bufferNotFull_.notify_all();

            MODULE_INFO(DataReceiver, "HardwareReceiver resumed");

            return SystemErrors::SUCCESS;
        }

        ErrorCode HardwareReceiver::cleanup()
        {
            MODULE_INFO(DataReceiver, "Cleaning up HardwareReceiver");

            // 确保停止接收
            if (state_ == ModuleState::RUNNING || state_ == ModuleState::PAUSED)
            {
                stop();
            }

            // 清理硬件资源
            if (hardwareDevice_)
            {
                cleanupHardware();
            }

            // 清理缓冲区
            {
                std::lock_guard<std::mutex> lock(bufferMutex_);
                while (!dataBuffer_.empty())
                {
                    dataBuffer_.pop();
                }
            }

            // 清理线程池
            receiverThreadPool_.reset();

            // 重置统计信息
            packetsReceived_ = 0;
            packetsDropped_ = 0;
            bytesReceived_ = 0;

            std::lock_guard<std::mutex> lock(stateMutex_);
            setState(ModuleState::UNINITIALIZED);

            MODULE_INFO(DataReceiver, "HardwareReceiver cleanup completed");

            return SystemErrors::SUCCESS;
        }

        //==============================================================================
        // IDataReceiver 接口实现
        //==============================================================================

        ErrorCode HardwareReceiver::configure(const DataReceiverConfig &config)
        {
            std::lock_guard<std::mutex> lock(configMutex_);

            MODULE_INFO(DataReceiver, "Configuring HardwareReceiver");

            // 验证配置参数
            if (config.packetSizeBytes == 0 || config.packetSizeBytes > MAX_PACKET_SIZE)
            {
                MODULE_ERROR(DataReceiver, "Invalid packet size: {}", config.packetSizeBytes);
                return SystemErrors::INVALID_PARAMETER;
            }

            if (config.maxQueueSize == 0 || config.maxQueueSize > MAX_QUEUE_SIZE)
            {
                MODULE_ERROR(DataReceiver, "Invalid queue size: {}", config.maxQueueSize);
                return SystemErrors::INVALID_PARAMETER;
            }

            // 如果正在运行，需要先停止
            if (state_ == ModuleState::RUNNING)
            {
                MODULE_WARN(DataReceiver, "Reconfiguring while running, stopping first");
                stop();
            }

            config_ = config;

            // 重新初始化（如果需要）
            if (state_ != ModuleState::UNINITIALIZED)
            {
                return initialize();
            }

            MODULE_INFO(DataReceiver, "HardwareReceiver configured successfully");

            return SystemErrors::SUCCESS;
        }

        ErrorCode HardwareReceiver::receivePacket(RawDataPacketPtr &packet, uint32_t timeoutMs)
        {
            // 检查状态
            if (state_ != ModuleState::RUNNING && state_ != ModuleState::PAUSED)
            {
                return DataReceiverErrors::RECEIVER_NOT_READY;
            }

            std::unique_lock<std::mutex> lock(bufferMutex_);

            // 等待数据或超时
            auto waitResult = bufferNotEmpty_.wait_for(
                lock,
                std::chrono::milliseconds(timeoutMs),
                [this]
                { return !dataBuffer_.empty() || shouldStop_; });

            if (shouldStop_)
            {
                return SystemErrors::OPERATION_TIMEOUT;
            }

            if (!waitResult)
            {
                return SystemErrors::OPERATION_TIMEOUT;
            }

            // 从缓冲区取出数据包
            packet = dataBuffer_.front();
            dataBuffer_.pop();

            // 通知生产者线程
            bufferNotFull_.notify_one();

            // 更新统计
            performanceMonitor_.lastReceiveTime = std::chrono::high_resolution_clock::now();

            return SystemErrors::SUCCESS;
        }

        std::future<RawDataPacketPtr> HardwareReceiver::receivePacketAsync()
        {
            return std::async(std::launch::async, [this]() -> RawDataPacketPtr
                              {
        RawDataPacketPtr packet;
        ErrorCode result = receivePacket(packet, 0);  // 无限等待

        if (isSuccess(result)) {
            return packet;
        }

        return nullptr; });
        }

        void HardwareReceiver::setPacketReceivedCallback(
            std::function<void(RawDataPacketPtr)> callback)
        {

            std::lock_guard<std::mutex> lock(callbackMutex_);
            packetReceivedCallback_ = callback;

            MODULE_DEBUG(DataReceiver, "Packet received callback set");
        }

        BufferStatus HardwareReceiver::getBufferStatus() const
        {
            std::lock_guard<std::mutex> lock(bufferMutex_);

            BufferStatus status;
            status.totalCapacity = config_.maxQueueSize;
            status.currentSize = static_cast<uint32_t>(dataBuffer_.size());
            status.peakSize = static_cast<uint32_t>(performanceMonitor_.peakBufferSize);
            status.totalReceived = packetsReceived_;
            status.totalDropped = packetsDropped_;

            return status;
        }

        ErrorCode HardwareReceiver::flushBuffer()
        {
            std::lock_guard<std::mutex> lock(bufferMutex_);

            MODULE_WARN(DataReceiver, "Flushing buffer, {} packets will be dropped",
                        dataBuffer_.size());

            while (!dataBuffer_.empty())
            {
                dataBuffer_.pop();
            }

            bufferNotFull_.notify_all();

            return SystemErrors::SUCCESS;
        }

        //==============================================================================
        // 私有方法实现 - 硬件管理
        //==============================================================================

        ErrorCode HardwareReceiver::initializeHardware()
        {
            MODULE_INFO(DataReceiver, "Initializing hardware device");

            // TODO: 实际硬件SDK集成
            // 这里是硬件初始化的框架代码

            try
            {
                // 1. 检测硬件设备
                if (!detectHardwareDevice())
                {
                    MODULE_ERROR(DataReceiver, "No hardware device detected");
                    return DataReceiverErrors::HARDWARE_FAILURE;
                }

                // 2. 打开设备
                // hardwareDevice_ = HardwareSDK::openDevice(0);

                // 3. 配置设备参数
                // HardwareSDK::configure(hardwareDevice_, ...);

                // 4. 启动DMA传输（如果支持）
                // HardwareSDK::startDMA(hardwareDevice_);

                MODULE_INFO(DataReceiver, "Hardware device initialized successfully");
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                MODULE_ERROR(DataReceiver, "Hardware initialization failed: {}", e.what());
                return DataReceiverErrors::HARDWARE_FAILURE;
            }
        }

        void HardwareReceiver::stopHardware()
        {
            if (hardwareDevice_)
            {
                MODULE_INFO(DataReceiver, "Stopping hardware device");

                // TODO: 停止硬件设备
                // HardwareSDK::stopDMA(hardwareDevice_);
                // HardwareSDK::closeDevice(hardwareDevice_);

                hardwareDevice_ = nullptr;
            }
        }

        void HardwareReceiver::cleanupHardware()
        {
            stopHardware();

            // TODO: 清理硬件资源
            // HardwareSDK::cleanup();

            MODULE_INFO(DataReceiver, "Hardware cleanup completed");
        }

        bool HardwareReceiver::detectHardwareDevice()
        {
            // TODO: 实际硬件检测
            // return HardwareSDK::getDeviceCount() > 0;

            // 模拟实现
            return false; // 当前总是返回false，使用模拟模式
        }

        ErrorCode HardwareReceiver::readHardwareData(RawDataPacketPtr &packet)
        {
            // TODO: 实际硬件数据读取
            // 这里是硬件数据读取的框架代码

            if (!hardwareDevice_)
            {
                return DataReceiverErrors::HARDWARE_FAILURE;
            }

            try
            {
                // 1. 分配数据包
                packet = std::make_shared<RawDataPacket>();

                // 2. 从硬件读取数据
                // size_t bytesRead = HardwareSDK::read(
                //     hardwareDevice_,
                //     packet->iqData.data(),
                //     config_.packetSizeBytes
                // );

                // 3. 设置数据包元信息
                packet->timestamp = std::chrono::high_resolution_clock::now();
                packet->sequenceId = ++lastSequenceId_;
                packet->priority = PacketPriority::NORMAL;

                // 4. 验证数据完整性
                if (!packet->isValid())
                {
                    return DataReceiverErrors::PACKET_CORRUPTION;
                }

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                MODULE_ERROR(DataReceiver, "Hardware read failed: {}", e.what());
                return DataReceiverErrors::DATA_SOURCE_ERROR;
            }
        }

        //==============================================================================
        // 私有方法实现 - 模拟数据生成
        //==============================================================================

        void HardwareReceiver::initializeSimulation()
        {
            MODULE_INFO(DataReceiver, "Initializing simulation mode");

            // 初始化随机数生成器
            randomGenerator_ = std::mt19937(simulationSeed_);
            noiseDistribution_ = std::normal_distribution<float>(0.0f, 1.0f);

            // 初始化模拟参数
            simulationParams_.centerFrequency = 10e9;    // 10 GHz
            simulationParams_.samplingFrequency = 100e6; // 100 MHz
            simulationParams_.pulseWidth = 1e-6;         // 1 微秒
            simulationParams_.noiseLevel = 0.1f;         // 噪声水平

            // 初始化模拟目标
            this->simulatedTargets_.clear();
            this->simulatedTargets_.push_back({100.0, 50.0, 1.0, 0.0, 0.0, 1.0});   // 距离100m，速度50m/s，RCS 1.0
            this->simulatedTargets_.push_back({200.0, -30.0, 0.5, 15.0, 0.0, 0.8}); // 距离200m，速度-30m/s，RCS 0.5
            this->simulatedTargets_.push_back({500.0, 0.0, 2.0, -10.0, 0.0, 1.2});  // 距离500m，静止，RCS 2.0

            RADAR_INFO("Simulation mode initialized with {} targets", this->simulatedTargets_.size());
        }

        RawDataPacketPtr HardwareReceiver::generateSimulatedPacket()
        {
            auto packet = std::make_shared<RawDataPacket>();

            // 设置数据包基本信息
            packet->timestamp = std::chrono::high_resolution_clock::now();
            packet->sequenceId = ++lastSequenceId_;
            packet->priority = PacketPriority::NORMAL;
            packet->channelCount = 4; // 模拟4通道
            packet->samplesPerChannel = config_.packetSizeBytes / (sizeof(ComplexFloat) * packet->channelCount);

            // 设置元信息
            packet->metadata.samplingFrequency = simulationParams_.samplingFrequency;
            packet->metadata.centerFrequency = simulationParams_.centerFrequency;
            packet->metadata.gain = 30.0;                    // 30 dB
            packet->metadata.pulseRepetitionInterval = 1000; // 1ms

            // 生成I/Q数据
            packet->iqData.resize(packet->channelCount * packet->samplesPerChannel);

            // 生成每个通道的数据
            for (uint32_t ch = 0; ch < packet->channelCount; ++ch)
            {
                generateChannelData(
                    packet->iqData.data() + ch * packet->samplesPerChannel,
                    packet->samplesPerChannel,
                    ch);
            }

            return packet;
        }

        void HardwareReceiver::generateChannelData(ComplexFloat *data, size_t samples, uint32_t channel)
        {
            // 生成基础噪声
            for (size_t i = 0; i < samples; ++i)
            {
                float noiseI = noiseDistribution_(randomGenerator_) * simulationParams_.noiseLevel;
                float noiseQ = noiseDistribution_(randomGenerator_) * simulationParams_.noiseLevel;
                data[i] = ComplexFloat(noiseI, noiseQ);
            }

            // 添加目标回波
            for (const auto &target : this->simulatedTargets_)
            {
                addTargetEcho(data, samples, channel, target);
            }

            // 添加杂波（可选）
            if (simulationParams_.clutterEnabled)
            {
                addClutter(data, samples);
            }
        }

        void HardwareReceiver::addTargetEcho(ComplexFloat *data, size_t samples,
                                             uint32_t channel, const SimulatedTarget &target)
        {
            // 计算目标参数
            const float c = 3e8f;                  // 光速
            float delay = 2.0f * target.range / c; // 双程延迟
            float dopplerShift = 2.0f * target.velocity * simulationParams_.centerFrequency / c;

            // 计算相位偏移（用于模拟阵列天线）
            float phaseOffset = channel * M_PI / 4.0f; // 45度相位差

            // 计算信号幅度（基于RCS）
            float amplitude = std::sqrt(target.rcs) * 0.5f;

            // 生成目标回波信号
            for (size_t i = 0; i < samples; ++i)
            {
                float t = i / simulationParams_.samplingFrequency;

                // 脉冲包络（矩形脉冲）
                if (t >= delay && t < delay + simulationParams_.pulseWidth)
                {
                    // 计算相位
                    float phase = 2.0f * M_PI * dopplerShift * t + phaseOffset;

                    // 添加到信号
                    data[i] += ComplexFloat(
                        amplitude * std::cos(phase),
                        amplitude * std::sin(phase));
                }
            }
        }

        void HardwareReceiver::addClutter(ComplexFloat *data, size_t samples)
        {
            // 生成地杂波（低频成分）
            std::uniform_real_distribution<float> clutterDist(0.0f, 0.2f);

            for (size_t i = 0; i < samples; ++i)
            {
                float clutterAmp = clutterDist(randomGenerator_);
                float phase = 2.0f * M_PI * i / samples;

                data[i] += ComplexFloat(
                    clutterAmp * std::cos(phase),
                    clutterAmp * std::sin(phase));
            }
        }

        //==============================================================================
        // 私有方法实现 - 缓冲区管理
        //==============================================================================

        ErrorCode HardwareReceiver::initializeBuffer()
        {
            MODULE_INFO(DataReceiver, "Initializing receive buffer with size {}",
                        config_.maxQueueSize);

            // 清空缓冲区
            while (!dataBuffer_.empty())
            {
                dataBuffer_.pop();
            }

            // 预分配内存（可选，用于减少运行时分配）
            // 这里使用队列，不需要预分配

            return SystemErrors::SUCCESS;
        }

        bool HardwareReceiver::pushToBuffer(RawDataPacketPtr packet)
        {
            std::unique_lock<std::mutex> lock(bufferMutex_);

            // 检查缓冲区是否已满
            if (dataBuffer_.size() >= config_.maxQueueSize)
            {
                // 根据溢出策略处理
                if (config_.overflowPolicy == "drop_oldest")
                {
                    // 丢弃最旧的数据包
                    dataBuffer_.pop();
                    packetsDropped_++;
                    MODULE_WARN(DataReceiver, "Buffer overflow, dropping oldest packet");
                }
                else if (config_.overflowPolicy == "drop_newest")
                {
                    // 丢弃新数据包
                    packetsDropped_++;
                    MODULE_WARN(DataReceiver, "Buffer overflow, dropping newest packet");
                    return false;
                }
                else
                {
                    // 等待缓冲区有空间
                    bufferNotFull_.wait(lock, [this]
                                        { return dataBuffer_.size() < config_.maxQueueSize || shouldStop_; });

                    if (shouldStop_)
                    {
                        return false;
                    }
                }
            }

            // 添加到缓冲区
            dataBuffer_.push(packet);

            // 更新统计
            packetsReceived_++;
            bytesReceived_ += packet->getDataSize();

            // 更新峰值
            if (dataBuffer_.size() > performanceMonitor_.peakBufferSize)
            {
                performanceMonitor_.peakBufferSize = dataBuffer_.size();
            }

            // 通知等待的消费者
            bufferNotEmpty_.notify_one();

            // 触发回调（如果设置）
            if (packetReceivedCallback_)
            {
                lock.unlock(); // 释放锁后再调用回调
                std::lock_guard<std::mutex> callbackLock(callbackMutex_);
                if (packetReceivedCallback_)
                {
                    packetReceivedCallback_(packet);
                }
            }

            return true;
        }

        //==============================================================================
        // 私有方法实现 - 线程函数
        //==============================================================================

        void HardwareReceiver::receiverThreadFunction()
        {
            MODULE_INFO(DataReceiver, "Receiver thread started");

            while (!shouldStop_)
            {
                if (!isReceiving_)
                {
                    // 暂停状态，等待恢复
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                try
                {
                    RawDataPacketPtr packet;
                    ErrorCode result;

                    if (config_.simulationEnabled)
                    {
                        // 模拟模式：生成模拟数据
                        packet = generateSimulatedPacket();
                        result = SystemErrors::SUCCESS;

                        // 模拟数据生成速率控制
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(config_.generationIntervalMs));
                    }
                    else
                    {
                        // 硬件模式：从硬件读取数据
                        result = readHardwareData(packet);
                    }

                    if (isSuccess(result) && packet)
                    {
                        // 添加到缓冲区
                        if (!pushToBuffer(packet))
                        {
                            MODULE_WARN(DataReceiver, "Failed to push packet to buffer");
                        }

                        // 更新性能统计
                        updatePerformanceMetrics();
                    }
                    else if (result != SystemErrors::OPERATION_TIMEOUT)
                    {
                        MODULE_ERROR(DataReceiver, "Failed to receive packet: 0x{:X}", result);

                        // 错误恢复策略
                        handleReceiveError(result);
                    }
                }
                catch (const std::exception &e)
                {
                    MODULE_ERROR(DataReceiver, "Exception in receiver thread: {}", e.what());
                }
            }

            MODULE_INFO(DataReceiver, "Receiver thread stopped");
        }

        void HardwareReceiver::monitorThreadFunction()
        {
            MODULE_INFO(DataReceiver, "Monitor thread started");

            while (!shouldStop_)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));

                if (isReceiving_)
                {
                    // 输出性能统计
                    logPerformanceMetrics();

                    // 检查缓冲区健康状态
                    checkBufferHealth();
                }
            }

            MODULE_INFO(DataReceiver, "Monitor thread stopped");
        }

        //==============================================================================
        // 私有方法实现 - 性能监控
        //==============================================================================

        void HardwareReceiver::updatePerformanceMetrics()
        {
            auto now = std::chrono::high_resolution_clock::now();

            // 更新吞吐量统计
            performanceMonitor_.packetsPerSecond = calculateThroughput();

            // 更新延迟统计
            if (performanceMonitor_.lastReceiveTime != Timestamp{})
            {
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                                   now - performanceMonitor_.lastReceiveTime)
                                   .count();

                performanceMonitor_.averageLatencyUs =
                    (performanceMonitor_.averageLatencyUs * 0.9) + (latency * 0.1);
            }

            performanceMonitor_.lastReceiveTime = now;
        }

        void HardwareReceiver::logPerformanceMetrics()
        {
            BufferStatus status = getBufferStatus();

            MODULE_INFO(DataReceiver,
                        "Performance: Packets/s={:.2f}, AvgLatency={:.2f}us, "
                        "Buffer={}/{}, Received={}, Dropped={}",
                        performanceMonitor_.packetsPerSecond,
                        performanceMonitor_.averageLatencyUs,
                        status.currentSize,
                        status.totalCapacity,
                        status.totalReceived,
                        status.totalDropped);
        }

        double HardwareReceiver::calculateThroughput()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                                now - performanceMonitor_.startTime)
                                .count();

            if (duration > 0)
            {
                return static_cast<double>(packetsReceived_) / duration;
            }

            return 0.0;
        }

        //==============================================================================
        // 私有方法实现 - 错误处理
        //==============================================================================

        void HardwareReceiver::handleReceiveError(ErrorCode error)
        {
            // 增加错误计数
            performanceMonitor_.errorCount++;

            // 根据错误类型采取不同的恢复策略
            switch (error)
            {
            case DataReceiverErrors::HARDWARE_FAILURE:
                MODULE_ERROR(DataReceiver, "Hardware failure detected, attempting recovery");
                attemptHardwareRecovery();
                break;

            case DataReceiverErrors::PACKET_CORRUPTION:
                MODULE_WARN(DataReceiver, "Packet corruption detected, skipping packet");
                // 继续接收下一个数据包
                break;

            case DataReceiverErrors::BUFFER_OVERFLOW:
                MODULE_WARN(DataReceiver, "Buffer overflow, clearing old data");
                // 缓冲区溢出已经在pushToBuffer中处理
                break;

            default:
                MODULE_ERROR(DataReceiver, "Unknown error: 0x{:X}", error);
                break;
            }

            // 如果错误次数过多，触发错误回调
            if (performanceMonitor_.errorCount > MAX_ERROR_COUNT)
            {
                if (errorCallback_)
                {
                    errorCallback_(error, "Too many receive errors");
                }

                // 重置错误计数
                performanceMonitor_.errorCount = 0;
            }
        }

        void HardwareReceiver::attemptHardwareRecovery()
        {
            MODULE_INFO(DataReceiver, "Attempting hardware recovery");

            if (!config_.simulationEnabled && hardwareDevice_)
            {
                // 停止硬件
                stopHardware();

                // 等待一段时间
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // 重新初始化硬件
                ErrorCode result = initializeHardware();
                if (isSuccess(result))
                {
                    MODULE_INFO(DataReceiver, "Hardware recovery successful");
                }
                else
                {
                    MODULE_ERROR(DataReceiver, "Hardware recovery failed");

                    // 切换到模拟模式作为备份
                    MODULE_WARN(DataReceiver, "Switching to simulation mode");
                    config_.simulationEnabled = true;
                    initializeSimulation();
                }
            }
        }

        void HardwareReceiver::checkBufferHealth()
        {
            BufferStatus status = getBufferStatus();

            // 检查缓冲区使用率
            float usage = static_cast<float>(status.currentSize) / status.totalCapacity;

            if (usage > 0.9f)
            {
                MODULE_WARN(DataReceiver, "Buffer usage critical: {:.1f}%", usage * 100);
            }
            else if (usage > 0.7f)
            {
                MODULE_DEBUG(DataReceiver, "Buffer usage high: {:.1f}%", usage * 100);
            }

            // 检查丢包率
            if (status.totalReceived > 0)
            {
                float dropRate = static_cast<float>(status.totalDropped) /
                                 (status.totalReceived + status.totalDropped);

                if (dropRate > 0.01f)
                { // 1%丢包率阈值
                    MODULE_WARN(DataReceiver, "High packet drop rate: {:.2f}%", dropRate * 100);
                }
            }
        }

        //==============================================================================
        // 私有方法实现 - 状态管理
        //==============================================================================

        void HardwareReceiver::setState(ModuleState newState)
        {
            ModuleState oldState = state_.exchange(newState);

            if (oldState != newState)
            {
                MODULE_DEBUG(DataReceiver, "State transition: {} -> {}",
                             static_cast<int>(oldState), static_cast<int>(newState));

                // 触发状态变化回调
                if (stateChangeCallback_)
                {
                    stateChangeCallback_(oldState, newState);
                }
            }
        }

        // 添加缺失的IModule方法实现
        void HardwareReceiver::setStateChangeCallback(StateChangeCallback callback)
        {
            stateChangeCallback_ = callback;
        }

        void HardwareReceiver::setErrorCallback(ErrorCallback callback)
        {
            errorCallback_ = callback;
        }

        PerformanceMetricsPtr HardwareReceiver::getPerformanceMetrics() const
        {
            return std::make_shared<SystemPerformanceMetrics>();
        }

    } // namespace modules
} // namespace radar
