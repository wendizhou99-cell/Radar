/**
 * @file logger.h
 * @brief 雷达MVP系统统一日志管理组件
 *
 * 基于spdlog构建的高性能异步日志系统，提供统一的日志接口和管理功能。
 * 支持多种输出目标（控制台、文件、网络）和日志级别控制。
 * 采用RAII设计模式确保资源安全管理。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see config_manager.h
 * @see error_codes.h
 */

#pragma once

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "common/error_codes.h"
#include "common/types.h"

namespace radar {
namespace common {

//==============================================================================
// 日志级别映射
//==============================================================================

/**
 * @brief 日志级别枚举
 * @details 定义系统支持的日志级别，与spdlog级别映射
 */
enum class LogLevel : uint8_t {
    TRACE = 0,  ///< 跟踪级别，详细的执行流程
    DEBUG,      ///< 调试级别，开发调试信息
    INFO,       ///< 信息级别，一般运行信息
    WARN,       ///< 警告级别，潜在问题提示
    ERR,        ///< 错误级别，错误但可恢复
    CRITICAL,   ///< 关键级别，严重错误
    OFF         ///< 关闭日志输出
};

/**
 * @brief 日志输出目标类型
 */
enum class LogSinkType : uint8_t {
    CONSOLE,        ///< 控制台输出
    FILE,           ///< 文件输出
    ROTATING_FILE,  ///< 轮转文件输出
    DAILY_FILE,     ///< 按日期轮转文件
    NETWORK         ///< 网络输出（预留）
};

//==============================================================================
// 日志配置结构
//==============================================================================

/**
 * @brief 日志系统配置参数
 * @details 控制日志系统的各项行为参数
 */
struct LoggerConfig {
    LogLevel globalLevel = LogLevel::INFO;  ///< 全局日志级别
    bool asyncMode = true;                  ///< 是否启用异步模式
    size_t asyncQueueSize = 8192;           ///< 异步队列大小
    size_t threadPoolSize = 1;              ///< 异步线程池大小

    /// 控制台输出配置
    struct ConsoleConfig {
        bool enabled = true;              ///< 是否启用控制台输出
        bool colorEnabled = true;         ///< 是否启用彩色输出
        LogLevel level = LogLevel::INFO;  ///< 控制台日志级别
    } console;

    /// 文件输出配置
    struct FileConfig {
        bool enabled = true;                          ///< 是否启用文件输出
        std::string filename = "logs/radar_mvp.log";  ///< 日志文件名
        size_t maxFileSize = 50 * 1024 * 1024;        ///< 最大文件大小(50MB)
        size_t maxFiles = 5;                          ///< 最大文件数量
        LogLevel level = LogLevel::DEBUG;             ///< 文件日志级别
    } file;

    /// 日志格式配置
    struct FormatConfig {
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";  ///< 日志格式模式
        bool flushImmediately = false;                                ///< 是否立即刷新缓冲区
        uint32_t flushIntervalSeconds = 3;                            ///< 自动刷新间隔(秒)
    } format;
};

//==============================================================================
// 日志统计信息结构
//==============================================================================

/**
 * @brief 日志系统统计信息
 */
struct LoggerStatistics {
    size_t totalLoggers;          ///< 总记录器数量
    size_t totalMessagesLogged;   ///< 总日志消息数
    size_t currentQueueSize;      ///< 当前队列大小
    bool isAsyncMode;             ///< 是否异步模式
    LogLevel currentGlobalLevel;  ///< 当前全局级别
};

//==============================================================================
// 日志管理器类
//==============================================================================

/**
 * @brief 统一日志管理器
 * @details
 * 单例模式的日志管理器，负责：
 * - 日志系统的初始化和配置
 * - 多个日志记录器的管理
 * - 运行时日志级别调整
 * - 日志输出目标的动态管理
 *
 * @note 线程安全，支持多线程并发访问
 * @warning 必须在使用任何日志功能前调用initialize()
 */
class LoggerManager {
  public:
    /**
     * @brief 获取日志管理器单例实例
     * @return 日志管理器引用
     */
    static LoggerManager &getInstance();

    /**
     * @brief 初始化日志系统
     * @param config 日志配置参数
     * @return 操作结果错误码
     * @throws SystemException 初始化失败时抛出异常
     */
    ErrorCode initialize(const LoggerConfig &config);

    /**
     * @brief 关闭日志系统
     * @return 操作结果错误码
     * @note 清理所有资源，关闭文件句柄
     */
    ErrorCode shutdown();

    /**
     * @brief 获取指定名称的日志记录器
     * @param name 日志记录器名称
     * @return 日志记录器智能指针
     * @note 如果记录器不存在，将自动创建
     */
    std::shared_ptr<spdlog::logger> getLogger(const std::string &name = "default");

    /**
     * @brief 创建模块专用日志记录器
     * @param moduleName 模块名称
     * @param level 模块专用日志级别（可选）
     * @return 日志记录器智能指针
     */
    std::shared_ptr<spdlog::logger> createModuleLogger(const std::string &moduleName, LogLevel level = LogLevel::INFO);

    /**
     * @brief 设置全局日志级别
     * @param level 新的日志级别
     * @return 操作结果错误码
     */
    ErrorCode setGlobalLogLevel(LogLevel level);

    /**
     * @brief 设置指定记录器的日志级别
     * @param loggerName 记录器名称
     * @param level 新的日志级别
     * @return 操作结果错误码
     */
    ErrorCode setLoggerLevel(const std::string &loggerName, LogLevel level);

    /**
     * @brief 强制刷新所有日志缓冲区
     * @return 操作结果错误码
     */
    ErrorCode flushAll();

    /**
     * @brief 获取日志系统统计信息
     * @return 统计信息结构
     */
    LoggerStatistics getStatistics() const;

    /**
     * @brief 检查日志系统是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const {
        return initialized_;
    }

  private:
    LoggerManager() = default;
    ~LoggerManager();

    // 禁用拷贝和赋值
    LoggerManager(const LoggerManager &) = delete;
    LoggerManager &operator=(const LoggerManager &) = delete;

    /**
     * @brief 创建控制台输出sink
     * @return 控制台sink智能指针
     */
    std::shared_ptr<spdlog::sinks::sink> createConsoleSink();

    /**
     * @brief 创建文件输出sink
     * @return 文件sink智能指针
     */
    std::shared_ptr<spdlog::sinks::sink> createFileSink();

    /**
     * @brief 将自定义日志级别转换为spdlog级别
     * @param level 自定义日志级别
     * @return spdlog日志级别
     */
    spdlog::level::level_enum toSpdlogLevel(LogLevel level) const;

    /**
     * @brief 将spdlog级别转换为自定义日志级别
     * @param level spdlog日志级别
     * @return 自定义日志级别
     */
    LogLevel fromSpdlogLevel(spdlog::level::level_enum level) const;

  private:
    bool initialized_ = false;                                                  ///< 初始化状态
    LoggerConfig config_;                                                       ///< 日志配置
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;  ///< 日志记录器映射
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks_;                   ///< 输出sink列表
    mutable std::mutex mutex_;                                                  ///< 线程安全互斥锁

    // 统计信息
    mutable std::atomic<size_t> totalMessagesLogged_{0};  ///< 总消息数统计
};

//==============================================================================
// 便捷宏定义
//==============================================================================

/// 获取默认日志记录器
#define RADAR_LOGGER() radar::common::LoggerManager::getInstance().getLogger()

/// 获取模块日志记录器
#define RADAR_MODULE_LOGGER(module) radar::common::LoggerManager::getInstance().getLogger(#module)

/// 日志记录宏
#define RADAR_TRACE(...) SPDLOG_LOGGER_TRACE(RADAR_LOGGER(), __VA_ARGS__)
#define RADAR_DEBUG(...) SPDLOG_LOGGER_DEBUG(RADAR_LOGGER(), __VA_ARGS__)
#define RADAR_INFO(...) SPDLOG_LOGGER_INFO(RADAR_LOGGER(), __VA_ARGS__)
#define RADAR_WARN(...) SPDLOG_LOGGER_WARN(RADAR_LOGGER(), __VA_ARGS__)
#define RADAR_ERROR(...) SPDLOG_LOGGER_ERROR(RADAR_LOGGER(), __VA_ARGS__)
#define RADAR_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(RADAR_LOGGER(), __VA_ARGS__)

/// 模块日志记录宏
#define MODULE_TRACE(module, ...) SPDLOG_LOGGER_TRACE(RADAR_MODULE_LOGGER(module), __VA_ARGS__)
#define MODULE_DEBUG(module, ...) SPDLOG_LOGGER_DEBUG(RADAR_MODULE_LOGGER(module), __VA_ARGS__)
#define MODULE_INFO(module, ...) SPDLOG_LOGGER_INFO(RADAR_MODULE_LOGGER(module), __VA_ARGS__)
#define MODULE_WARN(module, ...) SPDLOG_LOGGER_WARN(RADAR_MODULE_LOGGER(module), __VA_ARGS__)
#define MODULE_ERROR(module, ...) SPDLOG_LOGGER_ERROR(RADAR_MODULE_LOGGER(module), __VA_ARGS__)
#define MODULE_CRITICAL(module, ...) SPDLOG_LOGGER_CRITICAL(RADAR_MODULE_LOGGER(module), __VA_ARGS__)

}  // namespace common
}  // namespace radar
