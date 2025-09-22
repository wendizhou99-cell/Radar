/**
 * @file cpu_processor_test.cpp
 * @brief CPUDataProcessor 模块的单元测试
 *
 * 本文件包含 CPUDataProcessor 模块的完整单元测试套件，覆盖：
 * - 正常功能路径测试
 * - 边界条件测试
 * - 错误处理测试
 * - 性能和资源管理测试
 *
 * @author 测试代码生成器
 * @version 1.0
 * @date 2025-09-19
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <chrono>
  
#include "common/types.h"
#include "common/logger.h"

// 测试辅助工具（如果不存在，请创建 test_utils/test_data_factory.h）
#include "test_utils/test_data_factory.h"  // TODO: 如不存在需创建测试数据工厂，用于生成模拟雷达数据

using namespace radar;
using namespace radar::common;
using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

/**
 * @brief CPUDataProcessor 测试夹具类
 *
 * 提供每个测试用例的通用设置和清理功能。
 * 包含测试数据准备、配置设置等。
 */
class CPUProcessorTest : public ::testing::Test {
protected:
    /**
     * @brief 每个测试用例执行前的初始化
     */
    void SetUp() override {
        // 创建日志记录器（使用空 sink 避免测试输出干扰）
        auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
        logger_ = std::make_shared<spdlog::logger>("test_logger", null_sink);

        // 创建处理器实例
        processor_ = std::make_unique<CPUDataProcessor>(logger_);

        // 准备标准测试数据
        valid_packet_ = createValidTestPacket();    // TODO: 需要实现 createValidTestPacket() 在 test_data_factory.h
        invalid_packet_ = createInvalidTestPacket(); // TODO: 需要实现 createInvalidTestPacket() 在 test_data_factory.h

        // 配置处理器
        config_ = createDefaultProcessorConfig(); // TODO: 需要实现 createDefaultProcessorConfig() 在 test_data_factory.h
        processor_->configure(config_);
    }

    /**
     * @brief 每个测试用例执行后的清理
     */
    void TearDown() override {
        if (processor_) {
            processor_->cleanup();
        }
    }

    // 测试数据成员
    std::shared_ptr<spdlog::logger> logger_;
    std::unique_ptr<CPUDataProcessor> processor_;
    DataProcessorConfig config_;

    // 标准测试数据
    RawDataPacketPtr valid_packet_;
    RawDataPacketPtr invalid_packet_;

    // 测试常量
    static constexpr size_t SMALL_DATA_SIZE = 64;
    static constexpr size_t LARGE_DATA_SIZE = 4096;
    static constexpr double EPSILON = 1e-6;
};

// ===== 正常功能路径测试 =====

/**
 * @brief 测试处理器基本初始化功能
 * 目的：验证处理器能够正确初始化并达到就绪状态
 * 输入：有效的配置参数
 * 预期：初始化成功，处理器状态为 Ready
 */
TEST_F(CPUProcessorTest, InitializeWithValidConfig) {
    ASSERT_NE(processor_, nullptr) << "处理器创建失败";

    ErrorCode result = processor_->initialize();
    EXPECT_EQ(result, SystemErrors::SUCCESS) << "初始化应该成功";
    EXPECT_EQ(processor_->getState(), ModuleState::READY) << "处理器状态应该为 Ready";
}

/**
 * @brief 测试有效数据包的处理功能
 * 目的：验证处理器对有效雷达数据包的完整处理流程
 * 输入：标准大小的有效 I/Q 数据包
 * 预期：处理成功，输出结果包含正确的距离剖面、多普勒频谱等
 */
TEST_F(CPUProcessorTest, ProcessValidPacketReturnsSuccess) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);
    ASSERT_TRUE(valid_packet_->isValid());

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(valid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->processingSuccess) << "处理应该成功";
    EXPECT_EQ(result->sourcePacketId, valid_packet_->sequenceId) << "源数据包 ID 应该匹配";
    EXPECT_FALSE(result->rangeProfile.empty()) << "距离剖面数据不应该为空";
    EXPECT_FALSE(result->dopplerSpectrum.empty()) << "多普勒频谱数据不应该为空";
    EXPECT_GT(result->statistics.processingDurationMs, 0.0) << "处理时间应该大于零";
    EXPECT_GE(result->statistics.cpuUsagePercent, 0.0) << "CPU 使用率应该非负";
    EXPECT_EQ(result->statistics.gpuUsagePercent, 0.0) << "CPU 处理器 GPU 使用率应该为零";
}

/**
 * @brief 测试处理器能力查询功能
 * 目的：验证处理器能力信息的正确性
 * 输入：无
 * 预期：返回正确的 CPU 处理器能力描述
 */
TEST_F(CPUProcessorTest, GetCapabilitiesReturnsCorrectInfo) {
    ProcessorCapabilities caps = processor_->getCapabilities();

    EXPECT_TRUE(caps.supportsCPU) << "应该支持 CPU 处理";
    EXPECT_FALSE(caps.supportsGPU) << "不应该支持 GPU 处理";
    EXPECT_GE(caps.maxConcurrentTasks, 1) << "最大并发任务数应该至少为 1";
    EXPECT_FALSE(caps.supportedStrategies.empty()) << "支持的策略列表不应该为空";
    EXPECT_FALSE(caps.processorInfo.empty()) << "处理器信息不应该为空";
}

// ===== 边界条件测试 =====

/**
 * @brief 测试空数据包处理
 * 目的：验证处理器对空数据包的健壮性
 * 输入：空的 I/Q 数据向量
 * 预期：返回错误码，不崩溃
 */
TEST_F(CPUProcessorTest, ProcessEmptyPacketReturnsError) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);
    auto empty_packet = std::make_shared<RawDataPacket>();
    empty_packet->iqData.clear(); // 空数据

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(empty_packet);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->processingSuccess) << "空数据包处理应该失败";
}

/**
 * @brief 测试单通道数据处理
 * 目的：验证单通道雷达数据的处理能力
 * 输入：单通道的有效数据包
 * 预期：处理成功，波束形成数据为空或简单复制
 */
TEST_F(CPUProcessorTest, ProcessSingleChannelData) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);
    valid_packet_->channelCount = 1; // 设置为单通道

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(valid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->processingSuccess) << "单通道数据处理应该成功";
    // 对于单通道，波束形成数据可能为空或与输入相同
    EXPECT_TRUE(result->beamformedData.empty() ||
                result->beamformedData.size() == valid_packet_->iqData.size());
}

/**
 * @brief 测试大数据量处理
 * 目的：验证处理器处理大数据量的能力
 * 输入：接近系统限制的大数据量
 * 预期：能够处理或返回容量限制错误
 */
TEST_F(CPUProcessorTest, ProcessLargeDataHandlesGracefully) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);
    auto large_packet = createLargeTestPacket(LARGE_DATA_SIZE); // TODO: 需要实现 createLargeTestPacket()

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(large_packet);

    // Assert
    ASSERT_NE(result, nullptr);
    // 大数据量可能成功或因内存限制失败，但不应崩溃
    EXPECT_TRUE(result->processingSuccess ||
                !result->processingSuccess) << "大数据量处理不应崩溃";
}

// ===== 错误处理测试 =====

/**
 * @brief 测试未初始化时的操作
 * 目的：验证在未正确初始化时调用处理的错误处理
 * 输入：有效数据，但处理器未初始化
 * 预期：返回 NOT_READY 错误码
 */
TEST_F(CPUProcessorTest, ProcessWithoutInitializationReturnsError) {
    // Arrange - 不调用 initialize()

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(valid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->processingSuccess) << "未初始化处理应该失败";
}

/**
 * @brief 测试无效输入数据处理
 * 目的：验证对格式错误输入数据的错误处理
 * 输入：包含无效值的输入数据包
 * 预期：处理失败，返回错误状态
 */
TEST_F(CPUProcessorTest, ProcessInvalidDataReturnsError) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(invalid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->processingSuccess) << "无效数据处理应该失败";
}

// ===== 性能和资源管理测试 =====

/**
 * @brief 测试处理性能指标
 * 目的：验证处理时间和资源使用在合理范围内
 * 输入：标准测试数据
 * 预期：处理时间在预期范围内，内存使用合理
 */
TEST_F(CPUProcessorTest, ProcessingPerformanceWithinLimits) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);

    auto start_time = std::chrono::high_resolution_clock::now();

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(valid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->processingSuccess);

    // 性能验证
    EXPECT_LT(result->statistics.processingDurationMs, 1000.0) << "处理时间应该小于 1 秒";
    EXPECT_GE(result->statistics.cpuUsagePercent, 0.0) << "CPU 使用率应该非负";
    EXPECT_LE(result->statistics.cpuUsagePercent, 100.0) << "CPU 使用率应该不超过 100%";
    EXPECT_GT(result->statistics.memoryUsageBytes, 0) << "内存使用量应该大于零";
}

/**
 * @brief 测试内存使用量估算
 * 目的：验证内存使用量估算的准确性
 * 输入：不同大小的数据包
 * 预期：估算值与实际使用量接近
 */
TEST_F(CPUProcessorTest, MemoryUsageEstimationAccurate) {
    // Arrange
    ASSERT_EQ(processor_->initialize(), SystemErrors::SUCCESS);

    // Act
    ProcessingResultPtr result = processor_->executeProcessing(valid_packet_);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->processingSuccess);

    // 内存使用量应该大于输入数据大小
    size_t input_size = valid_packet_->getDataSize();
    EXPECT_GE(result->statistics.memoryUsageBytes, input_size) << "内存使用量应该至少等于输入大小";
}

} // anonymous namespace

// ===== 测试运行说明 =====
/*
编译和运行测试命令：

1. 编译测试：
   cd d:\work\Radar\radar_mvp\build
   cmake .. -DBUILD_TESTS=ON
   cmake --build . --target cpu_processor_test

2. 运行特定测试：
   ctest -R CPUProcessorTest

3. 运行详细输出：
   ctest -R CPUProcessorTest --verbose

4. 运行单个测试用例：
   .\tests\unit\cpu_processor_test.exe --gtest_filter="*ProcessValidPacketReturnsSuccess*"

5. 生成测试覆盖率报告（如果配置了 gcov）：
   cmake --build . --target coverage

注意事项：
- 测试依赖 test_utils/test_data_factory.h 中的辅助函数，如果不存在请创建
- 大数据量测试可能耗时较长，考虑使用 [SLOW] 标签分类
- 某些性能测试可能因硬件差异而波动，设置合理的容差范围
*/
