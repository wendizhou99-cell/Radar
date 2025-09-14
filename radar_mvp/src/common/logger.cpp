/**
 * @file logger.cpp
 * @brief 日志管理器的具体实现
 *
 * 实现了基于spdlog的高性能异步日志系统，提供完整的生命周期管理
 * 和多输出目标支持。采用RAII模式确保资源正确释放。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include "common/logger.h"
#include <spdlog/sinks/daily_file_sink.h>
#include <filesystem>
#include <iostream>

namespace radar
{
    namespace common
    {

        //==============================================================================
        // 静态成员和辅助函数
        //==============================================================================

        LoggerManager &LoggerManager::getInstance()
        {
            static LoggerManager instance;
            return instance;
        }

        LoggerManager::~LoggerManager()
        {
            if (initialized_)
            {
                shutdown();
            }
        }

        //==============================================================================
        // 初始化和配置
        //==============================================================================

        ErrorCode LoggerManager::initialize(const LoggerConfig &config)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (initialized_)
            {
                std::cout << "Warning: Logger manager already initialized" << std::endl;
                return SystemErrors::SUCCESS;
            }

            try
            {
                config_ = config;

                // 创建日志目录
                if (config_.file.enabled)
                {
                    std::filesystem::path logPath(config_.file.filename);
                    std::filesystem::path logDir = logPath.parent_path();
                    if (!logDir.empty() && !std::filesystem::exists(logDir))
                    {
                        std::filesystem::create_directories(logDir);
                    }
                }

                // 初始化异步日志线程池
                if (config_.asyncMode)
                {
                    spdlog::init_thread_pool(config_.asyncQueueSize, config_.threadPoolSize);
                }

                // 创建输出sinks
                sinks_.clear();

                if (config_.console.enabled)
                {
                    auto consoleSink = createConsoleSink();
                    if (consoleSink)
                    {
                        sinks_.push_back(consoleSink);
                    }
                }

                if (config_.file.enabled)
                {
                    auto fileSink = createFileSink();
                    if (fileSink)
                    {
                        sinks_.push_back(fileSink);
                    }
                }

                if (sinks_.empty())
                {
                    std::cerr << "No sinks available for logger initialization" << std::endl;
                    return SystemErrors::CONFIGURATION_ERROR;
                }

                // 创建默认日志记录器
                std::cout << "Creating default logger..." << std::endl;
                auto defaultLogger = createModuleLogger("default", config_.globalLevel);
                if (!defaultLogger)
                {
                    std::cerr << "Failed to create default logger" << std::endl;
                    return SystemErrors::INITIALIZATION_FAILED;
                }

                std::cout << "Setting default logger..." << std::endl;

                // 设置默认记录器
                spdlog::set_default_logger(defaultLogger);

                std::cout << "Setting global pattern..." << std::endl;
                // 设置全局格式
                spdlog::set_pattern(config_.format.pattern);

                std::cout << "Setting flush policy..." << std::endl;
                // 设置刷新策略
                if (config_.format.flushImmediately)
                {
                    spdlog::flush_on(toSpdlogLevel(LogLevel::TRACE));
                }
                else
                {
                    spdlog::flush_every(std::chrono::seconds(config_.format.flushIntervalSeconds));
                }

                initialized_ = true;

                // 现在可以安全使用RADAR_*宏了
                RADAR_INFO("Logger system initialized successfully");
                RADAR_DEBUG("Async mode: {}, Queue size: {}, Thread pool size: {}",
                            config_.asyncMode, config_.asyncQueueSize, config_.threadPoolSize);

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
                return SystemErrors::INITIALIZATION_FAILED;
            }
        }

        ErrorCode LoggerManager::shutdown()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                return SystemErrors::SUCCESS;
            }

            try
            {
                RADAR_INFO("Shutting down logger system...");

                // 刷新所有日志
                spdlog::shutdown();

                // 清理资源
                loggers_.clear();
                sinks_.clear();

                initialized_ = false;

                std::cout << "Logger system shutdown completed" << std::endl;
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error during logger shutdown: " << e.what() << std::endl;
                return SystemErrors::SHUTDOWN_FAILED;
            }
        }

        //==============================================================================
        // 日志记录器管理
        //==============================================================================

        std::shared_ptr<spdlog::logger> LoggerManager::getLogger(const std::string &name)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                return nullptr;
            }

            auto it = loggers_.find(name);
            if (it != loggers_.end())
            {
                return it->second;
            }

            // 如果记录器不存在，创建新的
            return createModuleLogger(name);
        }

        std::shared_ptr<spdlog::logger> LoggerManager::createModuleLogger(
            const std::string &moduleName, LogLevel level)
        {

            if (sinks_.empty())
            {
                std::cerr << "createModuleLogger failed: sinks.size=" << sinks_.size() << std::endl;
                return nullptr;
            }

            try
            {
                std::cout << "Creating logger for module: " << moduleName << std::endl;
                std::shared_ptr<spdlog::logger> logger;

                if (config_.asyncMode)
                {
                    logger = std::make_shared<spdlog::async_logger>(
                        moduleName, sinks_.begin(), sinks_.end(),
                        spdlog::thread_pool(), spdlog::async_overflow_policy::block);
                }
                else
                {
                    logger = std::make_shared<spdlog::logger>(
                        moduleName, sinks_.begin(), sinks_.end());
                }

                logger->set_level(toSpdlogLevel(level));
                logger->set_pattern(config_.format.pattern);

                // 注册记录器
                spdlog::register_logger(logger);
                loggers_[moduleName] = logger;

                RADAR_DEBUG("Created module logger: {}", moduleName);
                return logger;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to create logger '" << moduleName << "': " << e.what() << std::endl;
                return nullptr;
            }
        }

        //==============================================================================
        // 运行时配置
        //==============================================================================

        ErrorCode LoggerManager::setGlobalLogLevel(LogLevel level)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                return SystemErrors::INITIALIZATION_FAILED;
            }

            try
            {
                config_.globalLevel = level;
                spdlog::set_level(toSpdlogLevel(level));

                RADAR_INFO("Global log level changed to: {}", static_cast<int>(level));
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Failed to set global log level: {}", e.what());
                return SystemErrors::CONFIGURATION_ERROR;
            }
        }

        ErrorCode LoggerManager::setLoggerLevel(const std::string &loggerName, LogLevel level)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = loggers_.find(loggerName);
            if (it == loggers_.end())
            {
                return SystemErrors::INVALID_PARAMETER;
            }

            try
            {
                it->second->set_level(toSpdlogLevel(level));
                RADAR_DEBUG("Logger '{}' level changed to: {}", loggerName, static_cast<int>(level));
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Failed to set logger '{}' level: {}", loggerName, e.what());
                return SystemErrors::CONFIGURATION_ERROR;
            }
        }

        ErrorCode LoggerManager::flushAll()
        {
            if (!initialized_)
            {
                return SystemErrors::INITIALIZATION_FAILED;
            }

            try
            {
                spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger)
                                  { logger->flush(); });
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Failed to flush all loggers: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        //==============================================================================
        // 统计信息
        //==============================================================================

        LoggerStatistics LoggerManager::getStatistics() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            LoggerStatistics stats;
            stats.totalLoggers = loggers_.size();
            stats.totalMessagesLogged = totalMessagesLogged_.load();
            stats.currentQueueSize = config_.asyncMode ? config_.asyncQueueSize : 0;
            stats.isAsyncMode = config_.asyncMode;
            stats.currentGlobalLevel = config_.globalLevel;

            return stats;
        }

        //==============================================================================
        // 私有辅助方法
        //==============================================================================

        std::shared_ptr<spdlog::sinks::sink> LoggerManager::createConsoleSink()
        {
            try
            {
                auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sink->set_level(toSpdlogLevel(config_.console.level));
                sink->set_pattern(config_.format.pattern);

                std::cout << "Console sink created successfully" << std::endl;
                return sink;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to create console sink: " << e.what() << std::endl;
                return nullptr;
            }
        }

        std::shared_ptr<spdlog::sinks::sink> LoggerManager::createFileSink()
        {
            try
            {
                auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config_.file.filename,
                    config_.file.maxFileSize,
                    config_.file.maxFiles);

                sink->set_level(toSpdlogLevel(config_.file.level));
                sink->set_pattern(config_.format.pattern);

                std::cout << "File sink created: " << config_.file.filename << std::endl;
                return sink;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to create file sink: " << e.what() << std::endl;
                return nullptr;
            }
        }

        spdlog::level::level_enum LoggerManager::toSpdlogLevel(LogLevel level) const
        {
            switch (level)
            {
            case LogLevel::TRACE:
                return spdlog::level::trace;
            case LogLevel::DEBUG:
                return spdlog::level::debug;
            case LogLevel::INFO:
                return spdlog::level::info;
            case LogLevel::WARN:
                return spdlog::level::warn;
            case LogLevel::ERR:
                return spdlog::level::err;
            case LogLevel::CRITICAL:
                return spdlog::level::critical;
            case LogLevel::OFF:
                return spdlog::level::off;
            default:
                return spdlog::level::info;
            }
        }

        LogLevel LoggerManager::fromSpdlogLevel(spdlog::level::level_enum level) const
        {
            switch (level)
            {
            case spdlog::level::trace:
                return LogLevel::TRACE;
            case spdlog::level::debug:
                return LogLevel::DEBUG;
            case spdlog::level::info:
                return LogLevel::INFO;
            case spdlog::level::warn:
                return LogLevel::WARN;
            case spdlog::level::err:
                return LogLevel::ERR;
            case spdlog::level::critical:
                return LogLevel::CRITICAL;
            case spdlog::level::off:
                return LogLevel::OFF;
            default:
                return LogLevel::INFO;
            }
        }

    } // namespace common
} // namespace radar
