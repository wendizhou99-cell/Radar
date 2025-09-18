```markdown
# 03_generate_unit_tests.prompt.md
目的：为指定函数或类生成 googletest 单元测试模板，覆盖正常、边界与错误码路径。

注意：仓库级约束请在 .github/copilot-instructions.md 查看，本模板不重复全局项。

---
Strong constraints（强约束 — 必须遵守）
- 生成的测试使用 GoogleTest（googletest）框架，文件路径建议 tests/unit_tests/{module}_test.cpp。
- 至少包含：正常路径、边界条件、两个错误码场景（若适用）。
- 测试中应使用仓库现有的测试辅助函数（如存在 createTestDataFile()），若不存在则注明需提供辅助函数名。
- 不实现被测函数主体；仅生成测试代码。

Weak constraints（弱约束 — 推荐）
- 为每个测试用例写一行说明（目的与输入）。
- 使用小型、可控的测试数据；避免大文件或真实硬件依赖（如果必须，写 mock 建议）。
- 提供如何在本地或 CI 中运行该测试的命令示例（例如：cmake --build build && ctest -R {test_name}）。

模板示例（复制后替换）
"请为以下函数生成 googletest 单元测试（输出为完整 .cpp 文件），要求覆盖正常、边界、两个错误码分支，并使用仓库测试工具（若需要 mock 请说明）。
函数签名/描述：
{PASTE_FUNCTION_SIGNATURE_OR_BRIEF}
只返回测试文件内容。"
```

```CPP
/**
 * @file {module_name}_test.cpp
 * @brief Unit tests for {ModuleName} module
 *
 * 本文件包含 {ModuleName} 模块的完整单元测试套件，覆盖：
 * - 正常功能路径测试
 * - 边界条件测试
 * - 错误处理测试
 * - 性能和资源管理测试
 *
 * @author 测试代码生成器
 * @version 1.0
 * @date 生成日期
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// 被测试模块的头文件
#include "modules/{module_name}/{module_name}.h"
#include "common/error_codes.h"
#include "common/types.h"

// 测试辅助工具
#include "test_utils/test_data_factory.h"  // TODO: 如不存在需创建测试数据工厂
#include "test_utils/mock_helpers.h"       // TODO: 如不存在需创建 Mock 辅助类

using namespace radar::modules;
using namespace radar::common;
using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

/**
 * @brief {ModuleName} 测试夹具类
 *
 * 提供每个测试用例的通用设置和清理功能。
 * 包含测试数据准备、Mock 对象配置等。
 */
class {ModuleName}Test : public ::testing::Test {
protected:
    /**
     * @brief 每个测试用例执行前的初始化
     */
    void SetUp() override {
        // TODO: 创建被测试对象
        config_ = create{ModuleName}Config();  // TODO: 需要实现配置工厂函数
        module_ = {ModuleName}Factory::create(config_);

        // TODO: 准备标准测试数据
        valid_input_data_ = createValidInputData();    // TODO: 需要实现测试数据工厂
        invalid_input_data_ = createInvalidInputData(); // TODO: 需要实现测试数据工厂

        // TODO: 配置 Mock 对象
        // mock_dependency_ = std::make_shared<StrictMock<MockDependency>>();
    }

    /**
     * @brief 每个测试用例执行后的清理
     */
    void TearDown() override {
        if (module_) {
            module_->cleanup();
        }
        // TODO: 清理测试资源
    }

    // 测试数据成员
    {ModuleName}Config config_;
    std::unique_ptr<I{ModuleName}> module_;

    // 标准测试数据
    InputDataType valid_input_data_;
    InputDataType invalid_input_data_;
    OutputDataType output_data_;

    // Mock 对象 (如需要)
    // std::shared_ptr<MockDependency> mock_dependency_;

    // 测试常量
    static constexpr size_t SMALL_DATA_SIZE = 64;
    static constexpr size_t LARGE_DATA_SIZE = 4096;
    static constexpr double EPSILON = 1e-6;
};

// ===== 正常功能路径测试 =====

/**
 * @brief 测试模块基本初始化功能
 * 目的：验证模块能够正确初始化并达到就绪状态
 * 输入：有效的配置参数
 * 预期：初始化成功，模块状态为 Ready
 */
TEST_F({ModuleName}Test, InitializeWithValidConfig) {
    // Arrange - 准备测试环境
    ASSERT_NE(module_, nullptr) << "模块创建失败";

    // Act - 执行被测试功能
    ErrorCode result = module_->initialize();

    // Assert - 验证结果
    EXPECT_EQ(result, SystemErrors::SUCCESS) << "初始化应该成功";
    EXPECT_TRUE(module_->isReady()) << "初始化后模块应该处于就绪状态";
    EXPECT_EQ(module_->getState(), ModuleState::Ready) << "模块状态应该为 Ready";
}

/**
 * @brief 详细测试用例示例：数据处理功能的完整测试
 * 目的：验证模块主要数据处理功能在正常条件下工作正确
 * 输入：标准大小的有效输入数据
 * 预期：处理成功，输出数据格式正确，性能指标在预期范围内
 */
TEST_F({ModuleName}Test, ProcessValidDataReturnsSuccess) {
    // ===== Arrange - 详细的测试准备 =====

    // 1. 确保模块已正确初始化
    ASSERT_EQ(module_->initialize(), SystemErrors::SUCCESS);
    ASSERT_TRUE(module_->isReady());

    // 2. 准备具体的测试输入数据
    InputDataType input_data;
    input_data.resize(SMALL_DATA_SIZE);

    // 填充有意义的测试数据 (例如：模拟雷达信号)
    for (size_t i = 0; i < SMALL_DATA_SIZE; ++i) {
        input_data[i] = ComplexFloat{
            static_cast<float>(std::sin(2.0 * M_PI * i / SMALL_DATA_SIZE)),  // I 分量
            static_cast<float>(std::cos(2.0 * M_PI * i / SMALL_DATA_SIZE))   // Q 分量
        };
    }

    // 3. 准备输出缓冲区
    OutputDataType output_data;
    output_data.resize(SMALL_DATA_SIZE);  // 假设输出大小与输入相同

    // 4. 可选：配置处理选项
    ProcessingOptions options;
    options.mode = ProcessingMode::HighAccuracy;  // TODO: 根据实际接口调整
    options.enableValidation = true;

    // 5. 记录性能基准时间点
    auto start_time = std::chrono::high_resolution_clock::now();

    // ===== Act - 执行被测试的核心功能 =====
    ErrorCode result = module_->process(input_data, output_data, &options);

    // ===== Assert - 全面的结果验证 =====

    // 1. 基本功能验证：检查返回码
    EXPECT_EQ(result, SystemErrors::SUCCESS)
        << "数据处理应该成功，错误码: 0x" << std::hex << static_cast<uint32_t>(result);

    // 2. 输出数据有效性验证
    EXPECT_EQ(output_data.size(), SMALL_DATA_SIZE)
        << "输出数据大小应该与输入一致";

    // 检查输出数据不全为零（说明确实进行了处理）
    bool has_non_zero = std::any_of(output_data.begin(), output_data.end(),
        [](const auto& value) { return std::abs(value) > EPSILON; });
    EXPECT_TRUE(has_non_zero) << "输出数据不应该全为零";

    // 3. 数学正确性验证（具体验证逻辑取决于算法）
    // 例如：对于 FFT 处理，可以验证 Parseval 定理
    // TODO: 根据具体算法添加数学验证
    /*
    double input_energy = calculateEnergy(input_data);
    double output_energy = calculateEnergy(output_data);
    EXPECT_NEAR(input_energy, output_energy, input_energy * 0.01)  // 1% 容差
        << "输入输出能量应该基本守恒（FFT情况下）";
    */

    // 4. 性能验证
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // 假设处理 64 个样本应该在 100 微秒内完成
    EXPECT_LT(duration.count(), 100)
        << "处理时间应该在性能要求范围内，实际耗时: " << duration.count() << " μs";

    // 5. 模块状态验证
    EXPECT_TRUE(module_->isReady()) << "处理后模块应该保持就绪状态";

    // 6. 统计信息验证
    auto stats = module_->getStatistics();
    EXPECT_GT(stats.processedCount, 0) << "处理统计计数应该增加";
    EXPECT_EQ(stats.errorCount, 0) << "错误计数应该为零";

    // 7. 验证没有资源泄漏（如果有相关接口）
    // TODO: 如果模块提供内存使用查询接口，在此验证
    /*
    auto memory_usage_after = module_->getMemoryUsage();
    EXPECT_LE(memory_usage_after, expected_max_memory)
        << "处理后内存使用应该在预期范围内";
    */
}

// ===== 边界条件测试 =====

/**
 * @brief 测试空输入数据处理
 * 目的：验证模块对空数据的健壮性
 * 输入：空的输入数据容器
 * 预期：返回相应错误码，不崩溃
 */
TEST_F({ModuleName}Test, ProcessEmptyDataReturnsError) {
    // ... 测试实现
}

/**
 * @brief 测试最大数据量处理
 * 目的：验证模块处理大数据量的能力
 * 输入：接近系统限制的大数据量
 * 预期：能够处理或返回容量限制错误
 */
TEST_F({ModuleName}Test, ProcessLargeDataHandlesGracefully) {
    // ... 测试实现
}

// ===== 错误处理测试 =====

/**
 * @brief 测试未初始化时的操作
 * 目的：验证在未正确初始化时调用操作的错误处理
 * 输入：有效数据，但模块未初始化
 * 预期：返回 NOT_INITIALIZED 错误码
 */
TEST_F({ModuleName}Test, ProcessWithoutInitializationReturnsError) {
    // Arrange - 不调用 initialize()
    OutputDataType output;

    // Act
    ErrorCode result = module_->process(valid_input_data_, output);

    // Assert
    EXPECT_EQ(result, {ModuleName}Errors::NOT_INITIALIZED);
    EXPECT_FALSE(module_->isReady());
}

/**
 * @brief 测试无效输入数据处理
 * 目的：验证对格式错误输入数据的错误处理
 * 输入：包含无效值的输入数据
 * 预期：返回 INVALID_INPUT 错误码
 */
TEST_F({ModuleName}Test, ProcessInvalidDataReturnsInputError) {
    // ... 测试实现
}

// ===== 配置和状态测试 =====

/**
 * @brief 测试配置更新功能
 * 目的：验证运行时配置更新功能
 */
TEST_F({ModuleName}Test, UpdateConfigurationChangesSettings) {
    // ... 测试实现
}

/**
 * @brief 测试状态查询功能
 * 目的：验证模块状态查询的准确性
 */
TEST_F({ModuleName}Test, GetStateReflectsActualState) {
    // ... 测试实现
}

// ===== 并发和线程安全测试 =====

/**
 * @brief 测试并发处理安全性
 * 目的：验证多线程环境下的数据竞争安全性
 */
TEST_F({ModuleName}Test, ConcurrentProcessingIsSafe) {
    // ... 测试实现（如果模块声明为线程安全）
}

// ===== 资源管理测试 =====

/**
 * @brief 测试资源清理功能
 * 目的：验证模块正确释放所有资源
 */
TEST_F({ModuleName}Test, CleanupReleasesAllResources) {
    // ... 测试实现
}

// ===== Mock 和集成测试 =====

/**
 * @brief 测试依赖模块交互
 * 目的：验证与其他模块的正确交互（使用 Mock）
 */
TEST_F({ModuleName}Test, InteractsCorrectlyWithDependencies) {
    // ... 使用 Mock 对象的测试实现
}

} // anonymous namespace

// ===== 测试运行说明 =====
/*
编译和运行测试命令：

1. 编译测试：
   cd build
   cmake .. -DBUILD_TESTING=ON
   cmake --build . --target {module_name}_test

2. 运行特定测试：
   ctest -R {ModuleName}Test

3. 运行详细输出：
   ctest -R {ModuleName}Test --verbose

4. 运行单个测试用例：
   ./tests/unit_tests/{module_name}_test --gtest_filter="*ProcessValidDataReturnsSuccess*"

5. 生成测试覆盖率报告（如果配置了 gcov）：
   cmake --build . --target coverage

注意事项：
- 某些测试可能需要 GPU 环境，在 CI 中可能需要跳过
- 大数据量测试可能耗时较长，考虑使用 [SLOW] 标签分类
- Mock 对象需要根据实际依赖接口创建
*/
```
