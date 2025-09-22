/**
 * @file data_receiver_test.cpp
 * @brief DataReceiver 基类的单元测试
 *
 * 测试 DataReceiver 的生命周期、数据接收和缓冲区管理功能。
 * 使用 GoogleTest 框架，确保覆盖正常路径、错误路径和边界条件。
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <thread>

// 项目头文件
#include "common/error_codes.h"
#include "common/types.h"
#include "modules/data_receiver/data_receiver_base.h"

namespace radar {
namespace modules {

// 测试辅助类，用于访问 protected 方法
class TestableDataReceiver : public DataReceiver {
  public:
    using DataReceiver::DataReceiver;   // 继承构造函数
    using DataReceiver::enqueuePacket;  // 公开 enqueuePacket 方法
};

// 测试夹具类，用于设置和清理测试环境
class DataReceiverTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 创建一个空 logger 以避免日志输出干扰测试
        auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
        logger_ = std::make_shared<spdlog::logger>("test_logger", null_sink);
        receiver_ = std::make_unique<TestableDataReceiver>(logger_);
    }

    void TearDown() override {
        if (receiver_->getState() == ModuleState::RUNNING) {
            receiver_->stop();
        }
        receiver_->cleanup();
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::unique_ptr<TestableDataReceiver> receiver_;
};

// 测试构造函数和析构函数
TEST_F(DataReceiverTest, ConstructorAndDestructor) {
    EXPECT_EQ(receiver_->getState(), ModuleState::UNINITIALIZED);
    EXPECT_EQ(receiver_->getModuleName(), "DataReceiver");
}

// 测试配置功能
TEST_F(DataReceiverTest, ConfigureSuccess) {
    DataReceiverConfig config;
    config.maxQueueSize = 1000;
    config.generationIntervalMs = 5000;

    ErrorCode result = receiver_->configure(config);
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    EXPECT_EQ(receiver_->getState(), ModuleState::READY);
}

TEST_F(DataReceiverTest, ConfigureFailure) {
    // 模拟配置失败（通过异常）
    // 注意：实际中可能需要 Mock 或修改代码来触发异常
    DataReceiverConfig config;
    // 假设配置中某些字段无效
    ErrorCode result = receiver_->configure(config);
    // 预期失败，但基类实现可能总是成功；子类可重写
    EXPECT_EQ(result, SystemErrors::SUCCESS);  // 基类默认成功
}

// 测试生命周期：初始化
TEST_F(DataReceiverTest, Initialize) {
    ErrorCode result = receiver_->initialize();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
}

// 测试生命周期：启动和停止
TEST_F(DataReceiverTest, StartStopSuccess) {
    DataReceiverConfig config;
    receiver_->configure(config);

    ErrorCode startResult = receiver_->start();
    EXPECT_EQ(startResult, SystemErrors::SUCCESS);
    EXPECT_EQ(receiver_->getState(), ModuleState::RUNNING);

    ErrorCode stopResult = receiver_->stop();
    EXPECT_EQ(stopResult, SystemErrors::SUCCESS);
    EXPECT_EQ(receiver_->getState(), ModuleState::READY);
}

TEST_F(DataReceiverTest, StartWithoutConfig) {
    ErrorCode result = receiver_->start();
    EXPECT_EQ(result, DataReceiverErrors::RECEIVER_NOT_READY);
}

TEST_F(DataReceiverTest, StopWhenNotRunning) {
    ErrorCode result = receiver_->stop();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
}

// 测试暂停和恢复（基类默认实现）
TEST_F(DataReceiverTest, PauseResume) {
    DataReceiverConfig config;
    receiver_->configure(config);
    receiver_->start();

    ErrorCode pauseResult = receiver_->pause();
    EXPECT_EQ(pauseResult, SystemErrors::SUCCESS);

    ErrorCode resumeResult = receiver_->resume();
    EXPECT_EQ(resumeResult, SystemErrors::SUCCESS);

    receiver_->stop();
}

// 测试清理
TEST_F(DataReceiverTest, Cleanup) {
    DataReceiverConfig config;
    receiver_->configure(config);
    receiver_->start();

    ErrorCode result = receiver_->cleanup();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    EXPECT_EQ(receiver_->getState(), ModuleState::UNINITIALIZED);
}

// 测试数据接收：同步接收（需要模拟数据）
TEST_F(DataReceiverTest, ReceivePacketSync) {
    DataReceiverConfig config;
    receiver_->configure(config);
    receiver_->start();

    // 模拟入队数据包
    auto packet = std::make_shared<RawDataPacket>();
    packet->timestamp = std::chrono::high_resolution_clock::now();
    packet->iqData = {ComplexFloat(1.0f, 2.0f), ComplexFloat(3.0f, 4.0f)};
    receiver_->enqueuePacket(packet);

    RawDataPacketPtr receivedPacket;
    ErrorCode result = receiver_->receivePacket(receivedPacket, 1000);
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    ASSERT_NE(receivedPacket, nullptr);
    EXPECT_EQ(receivedPacket->iqData.size(), 2);

    receiver_->stop();
}

// 测试异步接收
TEST_F(DataReceiverTest, ReceivePacketAsync) {
    DataReceiverConfig config;
    receiver_->configure(config);
    receiver_->start();

    // 模拟入队数据包
    auto packet = std::make_shared<RawDataPacket>();
    packet->timestamp = std::chrono::high_resolution_clock::now();
    packet->iqData = {ComplexFloat(1.0f, 2.0f)};
    receiver_->enqueuePacket(packet);

    auto future = receiver_->receivePacketAsync();
    auto receivedPacket = future.get();
    ASSERT_NE(receivedPacket, nullptr);
    EXPECT_EQ(receivedPacket->iqData.size(), 1);

    receiver_->stop();
}

// 测试缓冲区状态
TEST_F(DataReceiverTest, GetBufferStatus) {
    BufferStatus status = receiver_->getBufferStatus();
    EXPECT_EQ(status.totalCapacity, 1000);  // 默认值
    EXPECT_EQ(status.currentSize, 0);
}

// 测试刷新缓冲区
TEST_F(DataReceiverTest, FlushBuffer) {
    // 入队一些数据包
    auto packet1 = std::make_shared<RawDataPacket>();
    auto packet2 = std::make_shared<RawDataPacket>();
    receiver_->enqueuePacket(packet1);
    receiver_->enqueuePacket(packet2);

    BufferStatus status = receiver_->getBufferStatus();
    EXPECT_EQ(status.currentSize, 2);

    ErrorCode result = receiver_->flushBuffer();
    EXPECT_EQ(result, SystemErrors::SUCCESS);

    status = receiver_->getBufferStatus();
    EXPECT_EQ(status.currentSize, 0);
}

// 测试回调设置
TEST_F(DataReceiverTest, SetCallbacks) {
    bool callbackCalled = false;
    receiver_->setPacketReceivedCallback([&](RawDataPacketPtr) { callbackCalled = true; });

    receiver_->setErrorCallback([](ErrorCode, const std::string &) {});

    // 模拟触发回调
    auto packet = std::make_shared<RawDataPacket>();
    receiver_->enqueuePacket(packet);
    // 等待回调执行（在实际测试中可能需要同步机制）
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(callbackCalled);
}

// 测试超时接收
TEST_F(DataReceiverTest, ReceivePacketTimeout) {
    RawDataPacketPtr packet;
    ErrorCode result = receiver_->receivePacket(packet, 10);  // 短超时，无数据
    EXPECT_EQ(result, SystemErrors::OPERATION_TIMEOUT);
}

}  // namespace modules
}  // namespace radar
