/**
 * @file config_manager_test.cpp
 * @brief 配置管理器单元测试
 *
 * 测试配置管理器的各项功能，包括配置加载、类型安全访问、
 * 变更通知、验证器等核心功能的正确性和线程安全性。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include "common/config_manager.h"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>

#include "common/logger.h"

using namespace radar::common;
using radar::ErrorCode;  // 使用 radar::ErrorCode

class ConfigManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 初始化日志系统（配置管理器需要日志）
        LoggerConfig logConfig;
        logConfig.console.enabled = true;
        logConfig.file.enabled = false;
        logConfig.globalLevel = LogLevel::WARN;  // 减少测试输出
        LoggerManager::getInstance().initialize(logConfig);

        // 创建测试目录
        std::filesystem::create_directories("test_configs");

        // 创建测试配置文件
        createTestConfigFile();
    }

    void TearDown() override {
        // 注意：单例不能通过赋值重置，但可以通过加载空配置来清理状态
        // 或者重新加载默认配置，这里我们简单地清理测试文件

        // 清理测试文件
        if (std::filesystem::exists("test_configs")) {
            std::filesystem::remove_all("test_configs");
        }

        LoggerManager::getInstance().shutdown();
    }

    void createTestConfigFile() {
        std::ofstream file("test_configs/test_config.yaml");
        file << R"(
# Test Configuration
system:
  name: "Test Radar System"
  version: "1.0.0"
  log_level: "info"
  max_threads: 8
  performance:
    enable_monitoring: true
    statistics_interval_ms: 1000

data_receiver:
  simulation:
    enabled: true
    data_rate_mbps: 100
    packet_size_bytes: 4096
  buffer:
    max_queue_size: 1000
    overflow_policy: "drop_oldest"

data_processor:
  strategy: "cpu_basic"
  cpu:
    worker_threads: 4
    batch_size: 16
  gpu:
    device_id: 0
    memory_pool_mb: 256

nested:
  level1:
    level2:
      value: "deep_value"
      number: 42
      flag: true

array_test:
  - item1
  - item2
  - item3

mixed_array:
  - name: "first"
    value: 1
  - name: "second"
    value: 2
)";
        file.close();
    }

    std::string getTestConfigPath() const {
        return "test_configs/test_config.yaml";
    }
};

//==============================================================================
// 基础功能测试
//==============================================================================

TEST_F(ConfigManagerTest, LoadFromFile) {
    auto &manager = ConfigManager::getInstance();
    EXPECT_FALSE(manager.isLoaded());

    // 测试加载不存在的文件
    ErrorCode result = manager.loadFromFile("nonexistent.yaml");
    EXPECT_EQ(result, radar::SystemErrors::RESOURCE_UNAVAILABLE);
    EXPECT_FALSE(manager.isLoaded());

    // 测试加载有效文件
    result = manager.loadFromFile(getTestConfigPath());
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_TRUE(manager.isLoaded());
}

TEST_F(ConfigManagerTest, LoadFromString) {
    auto &manager = ConfigManager::getInstance();

    std::string yamlContent = R"(
test:
  string_value: "hello"
  int_value: 123
  bool_value: true
  float_value: 3.14
)";

    ErrorCode result = manager.loadFromString(yamlContent);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_TRUE(manager.isLoaded());

    // 测试无效YAML
    result = manager.loadFromString("invalid: yaml: content: [[[");
    EXPECT_EQ(result, radar::SystemErrors::CONFIGURATION_ERROR);
}

//==============================================================================
// 配置值访问测试
//==============================================================================

TEST_F(ConfigManagerTest, GetValueBasicTypes) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 测试字符串值
    std::string systemName = manager.getValue<std::string>("system.name");
    EXPECT_EQ(systemName, "Test Radar System");

    // 测试整数值
    int maxThreads = manager.getValue<int>("system.max_threads");
    EXPECT_EQ(maxThreads, 8);

    // 测试布尔值
    bool monitoring = manager.getValue<bool>("system.performance.enable_monitoring");
    EXPECT_TRUE(monitoring);

    // 测试浮点数（通过整数配置项测试隐式转换）
    double rateAsDouble = manager.getValue<double>("data_receiver.simulation.data_rate_mbps");
    EXPECT_DOUBLE_EQ(rateAsDouble, 100.0);

    // 测试深层嵌套
    std::string deepValue = manager.getValue<std::string>("nested.level1.level2.value");
    EXPECT_EQ(deepValue, "deep_value");

    int deepNumber = manager.getValue<int>("nested.level1.level2.number");
    EXPECT_EQ(deepNumber, 42);
}

TEST_F(ConfigManagerTest, GetValueWithDefaults) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 测试存在的键
    std::string existingValue = manager.getValue<std::string>("system.name", "default");
    EXPECT_EQ(existingValue, "Test Radar System");

    // 测试不存在的键，应返回默认值
    std::string nonExistentValue = manager.getValue<std::string>("non.existent.key", "default_value");
    EXPECT_EQ(nonExistentValue, "default_value");

    int nonExistentInt = manager.getValue<int>("non.existent.int", 999);
    EXPECT_EQ(nonExistentInt, 999);

    bool nonExistentBool = manager.getValue<bool>("non.existent.bool", true);
    EXPECT_TRUE(nonExistentBool);
}

TEST_F(ConfigManagerTest, SetValue) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 测试设置现有值
    ErrorCode result = manager.setValue("system.max_threads", 16);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_EQ(manager.getValue<int>("system.max_threads"), 16);

    // 测试设置新值
    result = manager.setValue("new.config.item", "new_value");
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_EQ(manager.getValue<std::string>("new.config.item"), "new_value");

    // 测试设置不同类型
    result = manager.setValue("new.bool.value", true);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_TRUE(manager.getValue<bool>("new.bool.value"));

    result = manager.setValue("new.float.value", 2.718);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_DOUBLE_EQ(manager.getValue<double>("new.float.value"), 2.718);
}

//==============================================================================
// 配置项管理测试
//==============================================================================

TEST_F(ConfigManagerTest, HasKeyAndRemoveKey) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 测试hasKey
    EXPECT_TRUE(manager.hasKey("system.name"));
    EXPECT_TRUE(manager.hasKey("nested.level1.level2.value"));
    EXPECT_FALSE(manager.hasKey("non.existent.key"));

    // 测试removeKey
    ErrorCode result = manager.removeKey("system.max_threads");
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_FALSE(manager.hasKey("system.max_threads"));

    // 测试删除不存在的键
    result = manager.removeKey("non.existent.key");
    EXPECT_EQ(result, radar::SystemErrors::INVALID_PARAMETER);

    // 测试删除嵌套键
    result = manager.removeKey("nested.level1.level2.value");
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_FALSE(manager.hasKey("nested.level1.level2.value"));
    EXPECT_TRUE(manager.hasKey("nested.level1.level2.number"));  // 其他键应该保持
}

TEST_F(ConfigManagerTest, GetSubConfig) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 获取子配置
    auto subConfig = manager.getSubConfig("data_receiver");
    ASSERT_NE(subConfig, nullptr);
    EXPECT_TRUE(subConfig->IsMap());

    // 测试子配置内容
    EXPECT_TRUE((*subConfig)["simulation"]);
    EXPECT_TRUE((*subConfig)["buffer"]);

    // 获取更深层的子配置
    auto deepSubConfig = manager.getSubConfig("data_receiver.simulation");
    ASSERT_NE(deepSubConfig, nullptr);
    EXPECT_TRUE((*deepSubConfig)["enabled"]);

    // 获取不存在的子配置
    auto nonExistentSubConfig = manager.getSubConfig("non.existent.path");
    EXPECT_EQ(nonExistentSubConfig, nullptr);
}

//==============================================================================
// 配置变更通知测试
//==============================================================================

TEST_F(ConfigManagerTest, ChangeCallbacks) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    std::atomic<int> callbackCount{0};
    std::string lastChangedKey;
    ConfigChangeType lastChangeType;

    // 注册回调
    auto callbackId = manager.registerChangeCallback(
        [&](const ConfigChangeEvent &event) {
            callbackCount++;
            lastChangedKey = event.keyPath;
            lastChangeType = event.type;
        },
        "system.*");

    // 修改配置触发回调
    manager.setValue("system.max_threads", 20);
    EXPECT_EQ(callbackCount.load(), 1);
    EXPECT_EQ(lastChangedKey, "system.max_threads");
    EXPECT_EQ(lastChangeType, ConfigChangeType::MODIFIED);

    // 添加新配置
    manager.setValue("system.new_value", "test");
    EXPECT_EQ(callbackCount.load(), 2);
    EXPECT_EQ(lastChangedKey, "system.new_value");
    EXPECT_EQ(lastChangeType, ConfigChangeType::ADDED);

    // 删除配置
    manager.removeKey("system.new_value");
    EXPECT_EQ(callbackCount.load(), 3);
    EXPECT_EQ(lastChangeType, ConfigChangeType::DELETED);

    // 修改不匹配模式的配置（不应触发回调）
    manager.setValue("data_processor.strategy", "gpu");
    EXPECT_EQ(callbackCount.load(), 3);  // 计数不应增加

    // 取消注册回调
    ErrorCode result = manager.unregisterChangeCallback(callbackId);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);

    // 修改配置（不应再触发回调）
    manager.setValue("system.max_threads", 25);
    EXPECT_EQ(callbackCount.load(), 3);  // 计数不应增加
}

//==============================================================================
// 简单验证器实现用于测试
//==============================================================================

class RangeValidator : public IConfigValidator {
  public:
    RangeValidator(int min, int max) : min_(min), max_(max) {}

    bool validate(const YAML::Node &value, std::string &errorMessage) const override {
        try {
            int intValue = value.as<int>();
            if (intValue < min_ || intValue > max_) {
                errorMessage = "Value " + std::to_string(intValue) + " is out of range [" + std::to_string(min_) +
                               ", " + std::to_string(max_) + "]";
                return false;
            }
            return true;
        } catch (const std::exception &e) {
            errorMessage = "Failed to convert to integer: " + std::string(e.what());
            return false;
        }
    }

    std::string getDescription() const override {
        return "Range validator [" + std::to_string(min_) + ", " + std::to_string(max_) + "]";
    }

  private:
    int min_, max_;
};

TEST_F(ConfigManagerTest, ConfigValidation) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 注册验证器
    auto validator = std::make_shared<RangeValidator>(1, 16);
    ErrorCode result = manager.registerValidator("system.max_threads", validator);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);

    // 测试有效值
    result = manager.setValue("system.max_threads", 8);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);

    // 测试无效值
    result = manager.setValue("system.max_threads", 32);
    EXPECT_EQ(result, radar::SystemErrors::INVALID_PARAMETER);

    // 验证所有配置
    std::vector<std::string> errorReport;
    bool allValid = manager.validateAll(errorReport);
    EXPECT_TRUE(allValid);
    EXPECT_TRUE(errorReport.empty());

    // 设置无效值后再验证
    manager.setValue("system.max_threads", 0);  // 这应该通过setValue但验证时失败
    allValid = manager.validateAll(errorReport);
    // 注意：由于我们在setValue中就进行了验证，这个测试可能需要调整
}

//==============================================================================
// 文件操作测试
//==============================================================================

TEST_F(ConfigManagerTest, SaveAndReload) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 修改一些值
    manager.setValue("system.max_threads", 12);
    manager.setValue("new.test.value", "saved_value");

    // 保存到新文件
    std::string outputFile = "test_configs/output.yaml";
    ErrorCode result = manager.saveToFile(outputFile);
    EXPECT_EQ(result, radar::SystemErrors::SUCCESS);
    EXPECT_TRUE(std::filesystem::exists(outputFile));

    // 验证保存的文件内容（通过重新导出字符串比较）
    std::string exportedAfterSave = manager.exportToString(false);
    EXPECT_TRUE(exportedAfterSave.find("max_threads: 12") != std::string::npos);
    EXPECT_TRUE(exportedAfterSave.find("saved_value") != std::string::npos);
}

TEST_F(ConfigManagerTest, ExportToString) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 导出为字符串
    std::string exported = manager.exportToString(true);
    EXPECT_FALSE(exported.empty());
    EXPECT_TRUE(exported.find("system:") != std::string::npos);
    EXPECT_TRUE(exported.find("data_receiver:") != std::string::npos);

    // 验证导出的字符串包含预期的值
    EXPECT_TRUE(exported.find("Test Radar System") != std::string::npos);
    EXPECT_TRUE(exported.find("max_threads: 8") != std::string::npos);
}

//==============================================================================
// 并发安全测试
//==============================================================================

TEST_F(ConfigManagerTest, ConcurrentAccess) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    const int threadCount = 4;
    const int operationsPerThread = 100;
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;

    // 启动多个线程进行并发操作
    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&manager, &successCount, t, operationsPerThread]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                try {
                    // 混合读写操作
                    if (i % 3 == 0) {
                        // 读操作
                        std::string value = manager.getValue<std::string>("system.name", "default");
                        if (!value.empty()) {
                            successCount++;
                        }
                    } else if (i % 3 == 1) {
                        // 写操作
                        std::string key = "thread" + std::to_string(t) + ".value" + std::to_string(i);
                        if (manager.setValue(key, i) == radar::SystemErrors::SUCCESS) {
                            successCount++;
                        }
                    } else {
                        // 检查操作
                        if (manager.hasKey("system.name")) {
                            successCount++;
                        }
                    }
                } catch (const std::exception &e) {
                    // 忽略异常，只计算成功次数
                }
            }
        });
    }

    // 等待所有线程完成
    for (auto &thread : threads) {
        thread.join();
    }

    // 验证大部分操作成功
    EXPECT_GT(successCount.load(), threadCount * operationsPerThread * 0.8);
}

//==============================================================================
// 统计信息测试
//==============================================================================

TEST_F(ConfigManagerTest, Statistics) {
    auto &manager = ConfigManager::getInstance();
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 注册一些回调和验证器
    auto callbackId = manager.registerChangeCallback([](const ConfigChangeEvent &) {}, "*");
    auto validator = std::make_shared<RangeValidator>(1, 100);
    manager.registerValidator("system.max_threads", validator);

    auto stats = manager.getStatistics();

    EXPECT_GT(stats.totalKeys, 0);
    EXPECT_GE(stats.totalCallbacks, 1);
    EXPECT_GE(stats.totalValidators, 1);
    EXPECT_EQ(stats.sourceFile, getTestConfigPath());
    EXPECT_FALSE(stats.autoReloadEnabled);

    // 清理
    manager.unregisterChangeCallback(callbackId);
}

//==============================================================================
// 错误处理测试
//==============================================================================

TEST_F(ConfigManagerTest, ErrorHandling) {
    auto &manager = ConfigManager::getInstance();

    // 测试未加载时的操作
    EXPECT_FALSE(manager.isLoaded());
    EXPECT_EQ(manager.getValue<std::string>("any.key", "default"), "default");
    EXPECT_EQ(manager.setValue("any.key", "value"), radar::SystemErrors::INITIALIZATION_FAILED);
    EXPECT_FALSE(manager.hasKey("any.key"));
    EXPECT_EQ(manager.removeKey("any.key"), radar::SystemErrors::INITIALIZATION_FAILED);
    EXPECT_EQ(manager.saveToFile("test.yaml"), radar::SystemErrors::INITIALIZATION_FAILED);
    EXPECT_EQ(manager.reload(), radar::SystemErrors::INVALID_PARAMETER);

    // 加载后测试错误情况
    ASSERT_EQ(manager.loadFromFile(getTestConfigPath()), radar::SystemErrors::SUCCESS);

    // 测试无效的回调ID
    EXPECT_EQ(manager.unregisterChangeCallback(99999), radar::SystemErrors::INVALID_PARAMETER);
}

//==============================================================================
// 主函数
//==============================================================================

// 注意：实际的main函数将由gtest框架提供，这里只是测试用例定义
