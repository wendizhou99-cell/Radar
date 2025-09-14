/**
 * @file logger_test.cpp
 * @brief 日志系统单元测试
 *
 * 测试日志管理器的各项功能，包括初始化、日志记录、级别控制、
 * 多记录器管理等核心功能的正确性和性能。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include <gtest/gtest.h>
#include "common/logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace radar::common;
using radar::ErrorCode; // 使用 radar::ErrorCode

class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 清理测试环境
        LoggerManager::getInstance().shutdown();

        // 删除可能存在的测试日志文件
        if (std::filesystem::exists("test_logs"))
        {
            std::filesystem::remove_all("test_logs");
        }
    }

    void TearDown() override
    {
        LoggerManager::getInstance().shutdown();

        // 清理测试文件
        if (std::filesystem::exists("test_logs"))
        {
            std::filesystem::remove_all("test_logs");
        }
    }

    LoggerConfig createTestConfig()
    {
        LoggerConfig config;
        config.globalLevel = LogLevel::DEBUG;
        config.asyncMode = false; // 同步模式便于测试

        config.console.enabled = true;
        config.console.colorEnabled = false; // 避免测试输出中的颜色代码
        config.console.level = LogLevel::INFO;

        config.file.enabled = true;
        config.file.filename = "test_logs/test.log";
        config.file.maxFileSize = 1024 * 1024; // 1MB
        config.file.maxFiles = 3;
        config.file.level = LogLevel::DEBUG;

        config.format.pattern = "[%Y-%m-%d %H:%M:%S] [%l] %v";
        config.format.flushImmediately = true;

        return config;
    }
};

//==============================================================================
// 基础功能测试
//==============================================================================

TEST_F(LoggerTest, InitializationAndShutdown)
{
    auto &manager = LoggerManager::getInstance();
    EXPECT_FALSE(manager.isInitialized());

    // 测试初始化
    LoggerConfig config = createTestConfig();
    ErrorCode result = manager.initialize(config);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_TRUE(manager.isInitialized());

    // 测试重复初始化
    result = manager.initialize(config);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS); // 应该成功但给出警告

    // 测试关闭
    result = manager.shutdown();
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_FALSE(manager.isInitialized());
}

TEST_F(LoggerTest, DefaultLoggerCreation)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    // 获取默认日志记录器
    auto logger = manager.getLogger();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "default");

    // 多次获取应该返回同一个实例
    auto logger2 = manager.getLogger();
    EXPECT_EQ(logger, logger2);
}

TEST_F(LoggerTest, ModuleLoggerCreation)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    // 创建模块日志记录器
    auto dataLogger = manager.createModuleLogger("data_receiver", LogLevel::WARN);
    ASSERT_NE(dataLogger, nullptr);
    EXPECT_EQ(dataLogger->name(), "data_receiver");

    // 通过getLogger获取相同的记录器
    auto sameLogger = manager.getLogger("data_receiver");
    EXPECT_EQ(dataLogger, sameLogger);

    // 创建不同的模块记录器
    auto processLogger = manager.createModuleLogger("data_processor");
    ASSERT_NE(processLogger, nullptr);
    EXPECT_NE(dataLogger, processLogger);
}

//==============================================================================
// 日志级别测试
//==============================================================================

TEST_F(LoggerTest, LogLevelControl)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    config.globalLevel = LogLevel::INFO;
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    // 测试全局级别设置
    EXPECT_EQ(manager.setGlobalLogLevel(LogLevel::WARN), radar::SystemErrors::SUCCESS);

    // 测试模块级别设置
    auto logger = manager.createModuleLogger("test_module", LogLevel::DEBUG);
    ASSERT_NE(logger, nullptr);

    EXPECT_EQ(manager.setLoggerLevel("test_module", LogLevel::ERR), radar::SystemErrors::SUCCESS);
    EXPECT_EQ(manager.setLoggerLevel("nonexistent", LogLevel::ERR), radar::SystemErrors::INVALID_PARAMETER);
}

//==============================================================================
// 日志输出测试
//==============================================================================

TEST_F(LoggerTest, LogOutputToFile)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    auto logger = manager.getLogger();
    ASSERT_NE(logger, nullptr);

    // 写入测试消息
    logger->info("Test info message");
    logger->warn("Test warning message");
    logger->error("Test error message");

    // 强制刷新
    EXPECT_EQ(manager.flushAll(), radar::SystemErrors::SUCCESS);

    // 检查文件是否创建
    EXPECT_TRUE(std::filesystem::exists("test_logs/test.log"));

    // 读取文件内容验证
    std::ifstream logFile("test_logs/test.log");
    ASSERT_TRUE(logFile.is_open());

    std::string content((std::istreambuf_iterator<char>(logFile)),
                        std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.find("Test info message") != std::string::npos);
    EXPECT_TRUE(content.find("Test warning message") != std::string::npos);
    EXPECT_TRUE(content.find("Test error message") != std::string::npos);
}

TEST_F(LoggerTest, MacroUsage)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    // 测试便捷宏
    RADAR_INFO("This is an info message using macro");
    RADAR_WARN("This is a warning message: {}", 42);
    RADAR_ERROR("This is an error message with multiple params: {} {}", "test", 3.14);

    // 测试模块宏
    MODULE_INFO(test_module, "Module info message");
    MODULE_ERROR(test_module, "Module error message: {}", "error details");

    EXPECT_EQ(manager.flushAll(), radar::SystemErrors::SUCCESS);
}

//==============================================================================
// 性能和并发测试
//==============================================================================

TEST_F(LoggerTest, ConcurrentLogging)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    config.asyncMode = true; // 启用异步模式进行并发测试
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    const int threadCount = 4;
    const int messagesPerThread = 100;
    std::vector<std::thread> threads;

    // 启动多个线程同时写日志
    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&manager, t, messagesPerThread]()
                             {
            auto logger = manager.getLogger("thread_" + std::to_string(t));
            for (int i = 0; i < messagesPerThread; ++i) {
                logger->info("Thread {} message {}", t, i);
                if (i % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            } });
    }

    // 等待所有线程完成
    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(manager.flushAll(), radar::SystemErrors::SUCCESS);

    // 给异步日志一些时间完成写入
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

//==============================================================================
// 统计信息测试
//==============================================================================

TEST_F(LoggerTest, Statistics)
{
    auto &manager = LoggerManager::getInstance();
    LoggerConfig config = createTestConfig();
    ASSERT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);

    // 创建几个日志记录器
    auto logger1 = manager.createModuleLogger("module1");
    auto logger2 = manager.createModuleLogger("module2");
    auto logger3 = manager.getLogger("default");

    auto stats = manager.getStatistics();

    EXPECT_GE(stats.totalLoggers, 3);
    EXPECT_EQ(stats.isAsyncMode, config.asyncMode);
    EXPECT_EQ(stats.currentGlobalLevel, config.globalLevel);
}

//==============================================================================
// 错误处理测试
//==============================================================================

TEST_F(LoggerTest, ErrorHandling)
{
    auto &manager = LoggerManager::getInstance();

    // 测试未初始化时的操作
    EXPECT_FALSE(manager.isInitialized());
    EXPECT_EQ(manager.getLogger(), nullptr);
    EXPECT_EQ(manager.setGlobalLogLevel(LogLevel::INFO), radar::SystemErrors::INITIALIZATION_FAILED);
    EXPECT_EQ(manager.flushAll(), radar::SystemErrors::INITIALIZATION_FAILED);

    // 测试无效配置
    LoggerConfig invalidConfig;
    invalidConfig.console.enabled = false;
    invalidConfig.file.enabled = false; // 没有输出目标

    ErrorCode result = manager.initialize(invalidConfig);
    EXPECT_EQ(result, radar::SystemErrors::CONFIGURATION_ERROR);
}

//==============================================================================
// 配置测试
//==============================================================================

TEST_F(LoggerTest, ConfigurationOptions)
{
    auto &manager = LoggerManager::getInstance();

    // 测试不同的配置组合
    LoggerConfig config = createTestConfig();

    // 仅控制台输出
    config.file.enabled = false;
    EXPECT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);
    manager.shutdown();

    // 仅文件输出
    config = createTestConfig();
    config.console.enabled = false;
    EXPECT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);
    manager.shutdown();

    // 异步模式
    config = createTestConfig();
    config.asyncMode = true;
    config.asyncQueueSize = 1024;
    config.threadPoolSize = 2;
    EXPECT_EQ(manager.initialize(config), radar::SystemErrors::SUCCESS);
}

//==============================================================================
// 主函数
//==============================================================================

// 注意：实际的main函数将由gtest框架提供，这里只是测试用例定义
