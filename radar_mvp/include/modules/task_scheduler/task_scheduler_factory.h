/**
 * @file task_scheduler_factory.h
 * @brief 任务调度器工厂函数定义
 *
 * 定义任务调度器的创建工厂函数和相关工具。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#pragma once

#include <memory>

#include "task_scheduler_implementations.h"

namespace radar {

/**
 * @brief 任务调度器工厂命名空间
 */
namespace TaskSchedulerFactory {
/**
 * @brief 调度器类型枚举
 */
enum class SchedulerType {
    THREAD_POOL,   ///< 线程池调度器
    REAL_TIME,     ///< 实时调度器
    LOAD_BALANCE,  ///< 负载均衡调度器（预留）
    DISTRIBUTED    ///< 分布式调度器（预留）
};

/**
 * @brief 创建线程池调度器
 *
 * @param config 调度器配置
 * @param logger 日志记录器实例
 * @return 线程池调度器智能指针，创建失败时返回 nullptr
 */
std::unique_ptr<ThreadPoolScheduler> createThreadPoolScheduler(const TaskSchedulerConfig &config,
                                                               std::shared_ptr<spdlog::logger> logger = nullptr);

/**
 * @brief 创建实时调度器
 *
 * @param config 调度器配置
 * @param logger 日志记录器实例
 * @return 实时调度器智能指针，创建失败时返回 nullptr
 */
std::unique_ptr<RealTimeScheduler> createRealTimeScheduler(const TaskSchedulerConfig &config,
                                                           std::shared_ptr<spdlog::logger> logger = nullptr);

/**
 * @brief 自动创建合适的任务调度器
 *
 * @param schedulerType 调度器类型
 * @param config 调度器配置
 * @param logger 日志记录器实例
 * @return 任务调度器智能指针，创建失败时返回 nullptr
 */
std::unique_ptr<TaskScheduler> createScheduler(SchedulerType schedulerType, const TaskSchedulerConfig &config,
                                               std::shared_ptr<spdlog::logger> logger = nullptr);

/**
 * @brief 检查调度器类型是否可用
 *
 * @param schedulerType 调度器类型
 * @return 调度器类型是否可用
 */
bool isSchedulerTypeAvailable(SchedulerType schedulerType);

}  // namespace TaskSchedulerFactory

}  // namespace radar
