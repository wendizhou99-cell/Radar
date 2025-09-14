/**
 * @file interfaces.h
 * @brief 雷达MVP系统模块接口定义
 *
 * 定义了系统中所有核心模块的抽象接口，实现了接口与实现的分离。
 * 所有模块都继承自IModule基础接口，保证了统一的生命周期管理。
 * 采用策略模式设计，支持运行时算法切换和扩展。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see types.h
 * @see error_codes.h
 */

#pragma once

#include "types.h"
#include "error_codes.h"
#include <functional>
#include <future>
#include <memory>

namespace radar
{

    //==============================================================================
    // 回调函数类型定义
    //==============================================================================

    /// 数据处理完成回调函数
    using ProcessingCompleteCallback = std::function<void(const ProcessingResult &)>;

    /// 错误处理回调函数
    using ErrorCallback = std::function<void(ErrorCode, const std::string &)>;

    /// 状态变化回调函数
    using StateChangeCallback = std::function<void(ModuleState, ModuleState)>;

    /// 性能监控回调函数
    using PerformanceCallback = std::function<void(const SystemPerformanceMetrics &)>;

    //==============================================================================
    // 状态结构体定义
    //==============================================================================

    /**
     * @brief 缓冲区状态信息
     */
    struct BufferStatus
    {
        uint32_t totalCapacity; ///< 总容量
        uint32_t currentSize;   ///< 当前使用量
        uint32_t peakSize;      ///< 峰值使用量
        uint64_t totalReceived; ///< 累计接收数据包数
        uint64_t totalDropped;  ///< 累计丢弃数据包数
    };

    /**
     * @brief 基础模块接口
     */
    //==============================================================================

    /**
     * @brief 所有模块的基础接口
     * @details 定义了模块的通用生命周期管理和状态查询接口
     * @note 所有模块实现都必须继承此接口
     */
    class IModule
    {
    public:
        virtual ~IModule() = default;

        /**
         * @brief 初始化模块
         * @param config 模块配置参数（具体类型由子类定义）
         * @return 操作结果错误码
         * @throws RadarException 初始化失败时抛出异常
         *
         * @details
         * 模块初始化顺序：
         * 1. 验证配置参数有效性
         * 2. 分配必要的资源（内存、线程等）
         * 3. 建立外部连接（如果需要）
         * 4. 设置模块状态为READY
         */
        virtual ErrorCode initialize() = 0;

        /**
         * @brief 启动模块运行
         * @return 操作结果错误码
         * @throws ModuleException 启动失败时抛出异常
         *
         * @note 只有在READY状态下才能调用此方法
         */
        virtual ErrorCode start() = 0;

        /**
         * @brief 停止模块运行
         * @return 操作结果错误码
         * @note 停止后模块状态变为READY，可以重新启动
         */
        virtual ErrorCode stop() = 0;

        /**
         * @brief 暂停模块运行
         * @return 操作结果错误码
         * @note 暂停后可以通过resume()恢复运行
         */
        virtual ErrorCode pause() = 0;

        /**
         * @brief 恢复模块运行
         * @return 操作结果错误码
         * @note 只有在PAUSED状态下才能调用此方法
         */
        virtual ErrorCode resume() = 0;

        /**
         * @brief 清理模块资源
         * @return 操作结果错误码
         * @note 清理后模块状态变为UNINITIALIZED
         */
        virtual ErrorCode cleanup() = 0;

        /**
         * @brief 获取模块当前状态
         * @return 模块状态枚举
         */
        virtual ModuleState getState() const = 0;

        /**
         * @brief 获取模块名称
         * @return 模块名称字符串
         */
        virtual const std::string &getModuleName() const = 0;

        /**
         * @brief 设置状态变化回调函数
         * @param callback 状态变化回调函数
         */
        virtual void setStateChangeCallback(StateChangeCallback callback) = 0;

        /**
         * @brief 设置错误处理回调函数
         * @param callback 错误处理回调函数
         */
        virtual void setErrorCallback(ErrorCallback callback) = 0;

        /**
         * @brief 获取模块性能统计信息
         * @return 性能统计数据的智能指针
         */
        virtual PerformanceMetricsPtr getPerformanceMetrics() const = 0;
    };

    //==============================================================================
    // 数据接收模块接口
    //==============================================================================

    /**
     * @brief 数据接收模块接口
     * @details 负责从外部数据源接收雷达原始数据，支持硬件和模拟数据源
     */
    class IDataReceiver : public IModule
    {
    public:
        /**
         * @brief 配置数据接收参数
         * @param config 数据接收配置
         * @return 操作结果错误码
         */
        virtual ErrorCode configure(const DataReceiverConfig &config) = 0;

        /**
         * @brief 接收单个数据包（同步方式）
         * @param packet 输出参数，接收到的数据包
         * @param timeoutMs 超时时间（毫秒），0表示无限等待
         * @return 操作结果错误码
         * @retval SystemErrors::SUCCESS 成功接收数据包
         * @retval DataReceiverErrors::RECEIVER_NOT_READY 接收器未就绪
         * @retval SystemErrors::OPERATION_TIMEOUT 接收超时
         */
        virtual ErrorCode receivePacket(RawDataPacketPtr &packet, uint32_t timeoutMs = 0) = 0;

        /**
         * @brief 异步接收数据包
         * @return 数据包的future对象
         * @note 调用者可以通过future获取结果或检查状态
         */
        virtual std::future<RawDataPacketPtr> receivePacketAsync() = 0;

        /**
         * @brief 设置数据包接收回调函数
         * @param callback 数据包接收完成回调
         * @note 设置后接收器将以异步模式工作
         */
        virtual void setPacketReceivedCallback(
            std::function<void(RawDataPacketPtr)> callback) = 0;

        /**
         * @brief 获取接收缓冲区状态
         * @return 缓冲区使用统计信息
         */
        virtual BufferStatus getBufferStatus() const = 0;

        /**
         * @brief 刷新接收缓冲区
         * @return 操作结果错误码
         * @warning 此操作将丢弃所有缓冲的数据包
         */
        virtual ErrorCode flushBuffer() = 0;
    };

    //==============================================================================
    // 数据处理模块接口
    //==============================================================================

    /**
     * @brief 数据处理模块接口
     * @details 负责对原始雷达数据进行信号处理，支持多种处理策略
     */
    class IDataProcessor : public IModule
    {
    public:
        /**
         * @brief 配置数据处理参数
         * @param config 数据处理配置
         * @return 操作结果错误码
         */
        virtual ErrorCode configure(const DataProcessorConfig &config) = 0;

        /**
         * @brief 处理单个数据包（同步方式）
         * @param inputPacket 输入数据包
         * @param result 输出处理结果
         * @return 操作结果错误码
         * @retval SystemErrors::SUCCESS 处理成功
         * @retval DataProcessorErrors::PROCESSOR_NOT_READY 处理器未就绪
         * @retval DataProcessorErrors::INVALID_INPUT_DATA 输入数据无效
         * @retval DataProcessorErrors::PROCESSING_FAILED 处理失败
         */
        virtual ErrorCode processPacket(const RawDataPacketPtr &inputPacket,
                                        ProcessingResultPtr &result) = 0;

        /**
         * @brief 异步处理数据包
         * @param inputPacket 输入数据包
         * @return 处理结果的future对象
         */
        virtual std::future<ProcessingResultPtr> processPacketAsync(
            const RawDataPacketPtr &inputPacket) = 0;

        /**
         * @brief 批量处理数据包
         * @param inputPackets 输入数据包列表
         * @param results 输出处理结果列表
         * @return 操作结果错误码
         * @note 批量处理可以提高GPU利用率和处理效率
         */
        virtual ErrorCode processBatch(
            const std::vector<RawDataPacketPtr> &inputPackets,
            std::vector<ProcessingResultPtr> &results) = 0;

        /**
         * @brief 设置处理完成回调函数
         * @param callback 处理完成回调
         */
        virtual void setProcessingCompleteCallback(ProcessingCompleteCallback callback) = 0;

        /**
         * @brief 切换处理策略
         * @param strategy 新的处理策略
         * @return 操作结果错误码
         * @note 策略切换可能涉及GPU资源重新分配
         */
        virtual ErrorCode switchStrategy(ProcessingStrategy strategy) = 0;

        /**
         * @brief 获取当前处理策略
         * @return 当前使用的处理策略
         */
        virtual ProcessingStrategy getCurrentStrategy() const = 0;

        /**
         * @brief 获取处理器能力信息
         * @return 处理器能力描述结构
         */
        virtual ProcessorCapabilities getCapabilities() const = 0;
    };

    //==============================================================================
    // 任务调度模块接口
    //==============================================================================

    /**
     * @brief 任务调度模块接口
     * @details 负责管理系统中的异步任务执行和资源调度
     */
    class ITaskScheduler : public IModule
    {
    public:
        /// 任务函数类型定义
        using Task = std::function<void()>;
        using TaskWithResult = std::function<ProcessingResultPtr()>;

        /**
         * @brief 配置任务调度参数
         * @param config 任务调度配置
         * @return 操作结果错误码
         */
        virtual ErrorCode configure(const TaskSchedulerConfig &config) = 0;

        /**
         * @brief 提交普通任务
         * @param task 任务函数
         * @param priority 任务优先级
         * @return 任务的future对象
         */
        virtual std::future<void> submitTask(Task task, PacketPriority priority = PacketPriority::NORMAL) = 0;

        /**
         * @brief 提交有返回值的任务
         * @param task 任务函数
         * @param priority 任务优先级
         * @return 任务结果的future对象
         */
        virtual std::future<ProcessingResultPtr> submitTaskWithResult(
            TaskWithResult task, PacketPriority priority = PacketPriority::NORMAL) = 0;

        /**
         * @brief 提交数据处理任务
         * @param processor 数据处理器指针
         * @param packet 数据包指针
         * @param priority 任务优先级
         * @return 处理结果的future对象
         */
        virtual std::future<ProcessingResultPtr> submitProcessingTask(
            std::shared_ptr<IDataProcessor> processor,
            RawDataPacketPtr packet,
            PacketPriority priority = PacketPriority::NORMAL) = 0;

        /**
         * @brief 等待所有任务完成
         * @param timeoutMs 超时时间（毫秒），0表示无限等待
         * @return 操作结果错误码
         */
        virtual ErrorCode waitForAllTasks(uint32_t timeoutMs = 0) = 0;

        /**
         * @brief 取消所有等待中的任务
         * @return 被取消的任务数量
         */
        virtual uint32_t cancelPendingTasks() = 0;

        /**
         * @brief 获取调度器状态信息
         * @return 调度器状态统计
         */
        virtual SchedulerStatus getSchedulerStatus() const = 0;
    };

    //==============================================================================
    // 显示控制模块接口
    //==============================================================================

    /**
     * @brief 显示控制模块接口
     * @details 负责处理结果的可视化显示和输出
     */
    class IDisplayController : public IModule
    {
    public:
        /// 显示格式枚举
        enum class DisplayFormat
        {
            CONSOLE_TEXT,  ///< 控制台文本输出
            CONSOLE_CHART, ///< 控制台图表输出
            FILE_CSV,      ///< CSV文件输出
            FILE_JSON,     ///< JSON文件输出
            FILE_BINARY,   ///< 二进制文件输出
            GRAPHICS_2D,   ///< 2D图形输出
            GRAPHICS_3D    ///< 3D图形输出
        };

        /**
         * @brief 显示处理结果
         * @param result 处理结果
         * @param format 显示格式
         * @return 操作结果错误码
         */
        virtual ErrorCode displayResult(const ProcessingResult &result,
                                        DisplayFormat format = DisplayFormat::CONSOLE_TEXT) = 0;

        /**
         * @brief 显示系统性能统计
         * @param metrics 性能指标
         * @param format 显示格式
         * @return 操作结果错误码
         */
        virtual ErrorCode displayMetrics(const SystemPerformanceMetrics &metrics,
                                         DisplayFormat format = DisplayFormat::CONSOLE_TEXT) = 0;

        /**
         * @brief 设置显示更新间隔
         * @param intervalMs 更新间隔（毫秒）
         * @return 操作结果错误码
         */
        virtual ErrorCode setUpdateInterval(uint32_t intervalMs) = 0;

        /**
         * @brief 启用/禁用自动刷新
         * @param enabled 是否启用自动刷新
         * @return 操作结果错误码
         */
        virtual ErrorCode setAutoRefresh(bool enabled) = 0;

        /**
         * @brief 保存显示内容到文件
         * @param filename 文件名
         * @param format 文件格式
         * @return 操作结果错误码
         */
        virtual ErrorCode saveToFile(const std::string &filename,
                                     DisplayFormat format = DisplayFormat::FILE_CSV) = 0;

        /**
         * @brief 清空显示内容
         * @return 操作结果错误码
         */
        virtual ErrorCode clearDisplay() = 0;

        /**
         * @brief 获取支持的显示格式列表
         * @return 支持的格式列表
         */
        virtual std::vector<DisplayFormat> getSupportedFormats() const = 0;
    };

    //==============================================================================
    // 工厂接口
    //==============================================================================

    /**
     * @brief 模块工厂接口
     * @details 使用抽象工厂模式创建各种模块实例
     */
    class IModuleFactory
    {
    public:
        virtual ~IModuleFactory() = default;

        /**
         * @brief 创建数据接收模块
         * @return 数据接收模块智能指针
         */
        virtual std::shared_ptr<IDataReceiver> createDataReceiver() = 0;

        /**
         * @brief 创建数据处理模块
         * @return 数据处理模块智能指针
         */
        virtual std::shared_ptr<IDataProcessor> createDataProcessor() = 0;

        /**
         * @brief 创建任务调度模块
         * @return 任务调度模块智能指针
         */
        virtual std::shared_ptr<ITaskScheduler> createTaskScheduler() = 0;

        /**
         * @brief 创建显示控制模块
         * @return 显示控制模块智能指针
         */
        virtual std::shared_ptr<IDisplayController> createDisplayController() = 0;
    };

} // namespace radar
