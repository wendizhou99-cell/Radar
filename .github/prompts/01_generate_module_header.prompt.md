```markdown
# 01_generate_module_header.prompt.md
目的：生成新的模块 header（interface），只输出 header 文件内容，便于分步开发。

注意：仓库级全局约束请参见 .github/copilot-instructions.md（此处不重复全局规则）。

---
Strong constraints（强约束 — 必须遵守）
- 仅生成 header（.h），不包含实现（.cpp）。
- 继承 radar::common::IModule 或指定的项目接口（如果模块需继承其他接口，明确写出）。
- 所有可失败方法返回 ErrorCode；不使用异常。
- 使用项目特定类型时请优先引用 include/common/types.h（但不在此处重复列出全局类型列表）。
- 为 public API 添加 Doxygen 注释模板（@brief/@param/@return）。

Weak constraints（弱约束 — 推荐）
- 命名风格遵循项目约定（namespace radar::modules、member_ 后缀等）。
- 在 header 顶部写简短模块说明（1-2 行）。
- 为每个 public 方法附带简单示例用法（可选）。

输入占位符（使用时替换）
- {MODULE_NAME}：模块名（PascalCase）
- {MODULE_BRIEF}：一句话描述模块职责
- {DEPENDENCIES}：需要引用的其他接口或类型（可留空）

模板示例提示（复制到 Copilot Chat 并替换占位符）
"为模块 {MODULE_NAME} 生成 header 文件（路径：include/modules/{module_name}.h）。
要求（强约束）：继承 radar::common::IModule；所有失败路径使用 ErrorCode，不要实现任何方法，只提供声明和必要的 Doxygen 注释。
弱约束：遵循项目命名规范，简短描述模块职责。只返回文件内容。"
```

```cpp
#pragma once

/**
 * @file {MODULE_NAME}.h
 * @brief {MODULE_BRIEF}
 *
 * 本模块提供{MODULE_BRIEF的详细描述}功能。
 * 所有操作都是异步安全的，支持多线程环境下的并发访问。
 *
 * @author 生成者名称
 * @version 1.0
 * @date 生成日期
 * @since 1.0
 *
 * @see IModule
 * @see {相关模块接口}
 */

#include "common/interfaces.h"
#include "common/error_codes.h"
#include "common/types.h"
{DEPENDENCIES}

namespace radar {
namespace modules {

/**
 * @brief {MODULE_BRIEF}
 *
 * {MODULE_NAME} 负责{详细职责描述}。主要功能包括：
 * - 功能点1：具体描述
 * - 功能点2：具体描述
 * - 功能点3：具体描述
 *
 * @details
 * 典型使用流程：
 * 1. 调用 initialize() 初始化模块
 * 2. 调用主要业务方法处理数据
 * 3. 通过 getState() 监控模块状态
 * 4. 调用 cleanup() 清理资源
 *
 * @note 该接口的实现必须是线程安全的
 * @warning 在调用业务方法前必须确保模块已成功初始化
 *
 * @since 1.0
 * @see IModule
 * @see {相关配置类}
 */
class I{MODULE_NAME} : public radar::common::IModule {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~I{MODULE_NAME}() = default;

    // ===== 核心业务接口 =====

    /**
     * @brief 主要业务处理方法
     *
     * 对输入数据执行{具体处理描述}操作。
     *
     * @param[in] input 输入数据，必须满足{输入要求}
     * @param[out] output 处理结果输出，成功时包含{输出内容描述}
     * @param[in] options 可选处理参数，nullptr 表示使用默认配置
     *
     * @return 处理状态码
     * @retval SystemErrors::SUCCESS 处理成功完成
     * @retval {MODULE_NAME}Errors::INVALID_INPUT 输入数据格式错误
     * @retval {MODULE_NAME}Errors::PROCESSING_FAILED 处理过程中发生错误
     * @retval {MODULE_NAME}Errors::OUTPUT_BUFFER_FULL 输出缓冲区不足
     *
     * @pre initialize() 必须已成功调用
     * @pre input 必须通过 validateInput() 验证
     * @post 成功时 output 包含有效的处理结果
     *
     * @note 该方法是线程安全的，支持并发调用
     * @warning 大数据量处理可能耗时较长
     *
     * @see initialize()
     * @see validateInput()
     * @see {相关配置类}
     *
     * @since 1.0
     *
     * @code
     * // 使用示例
     * auto module = {MODULE_NAME}Factory::create(config);
     * if (module->initialize() == SystemErrors::SUCCESS) {
     *     InputDataType input = prepareInput();
     *     OutputDataType output;
     *
     *     auto result = module->process(input, output);
     *     if (result == SystemErrors::SUCCESS) {
     *         handleResult(output);
     *     }
     * }
     * @endcode
     */
    virtual ErrorCode process(
        const InputDataType& input,
        OutputDataType& output,
        const ProcessingOptions* options = nullptr
    ) = 0;

    /**
     * @brief 验证输入数据有效性
     *
     * 检查输入数据是否符合处理要求。
     *
     * @param[in] input 待验证的输入数据
     *
     * @return 验证结果
     * @retval SystemErrors::SUCCESS 输入数据有效
     * @retval {MODULE_NAME}Errors::INVALID_FORMAT 数据格式不正确
     * @retval {MODULE_NAME}Errors::SIZE_MISMATCH 数据大小不匹配
     *
     * @note 该方法是无副作用的，可以安全地重复调用
     * @since 1.0
     */
    virtual ErrorCode validateInput(const InputDataType& input) const = 0;

    // ===== 状态查询接口 =====

    /**
     * @brief 获取模块当前状态
     *
     * @return 当前模块状态
     * @note 该方法是线程安全的
     * @since 1.0
     */
    virtual ModuleState getState() const override = 0;

    /**
     * @brief 检查模块是否就绪
     *
     * @return true 如果模块已初始化且可以处理数据，false 否则
     * @note 该方法是线程安全的
     * @since 1.0
     */
    virtual bool isReady() const = 0;

    /**
     * @brief 获取模块性能统计信息
     *
     * @return 当前性能统计数据
     * @note 该方法是线程安全的
     * @since 1.0
     */
    virtual PerformanceStatistics getStatistics() const = 0;

    // ===== 配置管理接口 =====

    /**
     * @brief 更新模块配置
     *
     * 动态更新模块运行参数。某些参数可能需要重新初始化才能生效。
     *
     * @param[in] config 新的配置参数
     *
     * @return 配置更新结果
     * @retval SystemErrors::SUCCESS 配置更新成功
     * @retval {MODULE_NAME}Errors::INVALID_CONFIG 配置参数无效
     * @retval {MODULE_NAME}Errors::UPDATE_FAILED 配置更新失败
     *
     * @note 某些配置更改可能需要调用 initialize() 重新初始化
     * @warning 在处理过程中更新配置可能导致不可预期的结果
     * @since 1.0
     */
    virtual ErrorCode updateConfiguration(const {MODULE_NAME}Config& config) = 0;

    /**
     * @brief 获取当前配置
     *
     * @return 当前模块配置的只读引用
     * @since 1.0
     */
    virtual const {MODULE_NAME}Config& getConfiguration() const = 0;

    // ===== IModule 接口实现 =====

    /**
     * @brief 初始化模块
     *
     * 执行模块的初始化操作，包括资源分配、设备准备等。
     *
     * @return 初始化结果
     * @retval SystemErrors::SUCCESS 初始化成功
     * @retval {MODULE_NAME}Errors::INIT_FAILED 初始化失败
     * @retval {MODULE_NAME}Errors::RESOURCE_UNAVAILABLE 所需资源不可用
     *
     * @post 成功时模块状态变为 ModuleState::Ready
     * @note 该方法不是线程安全的，应在单线程环境下调用
     * @since 1.0
     */
    virtual ErrorCode initialize() override = 0;

    /**
     * @brief 启动模块运行
     *
     * 启动模块的后台处理线程或任务。
     *
     * @return 启动结果
     * @retval SystemErrors::SUCCESS 启动成功
     * @retval {MODULE_NAME}Errors::START_FAILED 启动失败
     *
     * @pre initialize() 必须已成功调用
     * @post 成功时模块状态变为 ModuleState::Running
     * @since 1.0
     */
    virtual ErrorCode run() override = 0;

    /**
     * @brief 停止模块运行
     *
     * 优雅地停止模块运行，等待当前处理完成。
     *
     * @return 停止结果
     * @retval SystemErrors::SUCCESS 停止成功
     * @retval {MODULE_NAME}Errors::STOP_FAILED 停止失败
     *
     * @post 成功时模块状态变为 ModuleState::Stopped
     * @note 该方法会阻塞直到所有处理完成
     * @since 1.0
     */
    virtual ErrorCode stop() override = 0;

    /**
     * @brief 清理模块资源
     *
     * 释放模块占用的所有资源，包括内存、设备等。
     *
     * @post 模块状态变为 ModuleState::Uninitialized
     * @note 调用此方法后，需要重新 initialize() 才能使用模块
     * @since 1.0
     */
    virtual void cleanup() override = 0;
};

// ===== 相关类型定义 =====

/**
 * @brief {MODULE_NAME} 模块配置参数
 */
struct {MODULE_NAME}Config {
    // TODO: 根据具体模块需求定义配置参数
    // 示例：
    // double processingThreshold = 0.5;
    // size_t bufferSize = 1024;
    // ProcessingMode mode = ProcessingMode::HighPerformance;
};

/**
 * @brief {MODULE_NAME} 模块错误码
 */
namespace {MODULE_NAME}Errors {
    // TODO: 根据具体模块需求定义错误码
    // 错误码范围: 0x{模块ID}000 - 0x{模块ID}FFF
    constexpr ErrorCode INVALID_INPUT = 0x{模块ID}001;
    constexpr ErrorCode PROCESSING_FAILED = 0x{模块ID}002;
    constexpr ErrorCode INVALID_CONFIG = 0x{模块ID}003;
    // ... 更多错误码
}

/**
 * @brief {MODULE_NAME} 工厂类声明
 */
class {MODULE_NAME}Factory {
public:
    /**
     * @brief 创建 {MODULE_NAME} 实例
     *
     * @param config 模块配置参数
     * @return 模块实例的智能指针，失败时返回 nullptr
     */
    static std::unique_ptr<I{MODULE_NAME}> create(const {MODULE_NAME}Config& config);

    /**
     * @brief 创建默认配置的 {MODULE_NAME} 实例
     *
     * @return 模块实例的智能指针，失败时返回 nullptr
     */
    static std::unique_ptr<I{MODULE_NAME}> createDefault();
};

} // namespace modules
} // namespace radar
```
