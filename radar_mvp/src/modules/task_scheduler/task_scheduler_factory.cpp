/**
 * @file task_scheduler_factory.cpp
 * @brief 任务调度器工厂实现
 *
 * 实现任务调度器的创建工厂函数。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include "modules/task_scheduler/task_scheduler_factory.h"

#include "common/error_codes.h"
#include "common/logger.h"

namespace radar {

namespace TaskSchedulerFactory {

std::unique_ptr<ThreadPoolScheduler> createThreadPoolScheduler(const TaskSchedulerConfig &config,
                                                               std::shared_ptr<spdlog::logger> logger) {
    try {
        auto scheduler = std::make_unique<ThreadPoolScheduler>(config.coreThreads, logger);
        ErrorCode result = scheduler->configure(config);
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("Failed to configure thread pool scheduler: {}", static_cast<int>(result));
            return nullptr;
        }
        RADAR_INFO("Created thread pool scheduler with {} threads", config.coreThreads);
        return scheduler;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception creating thread pool scheduler: {}", e.what());
        return nullptr;
    }
}

std::unique_ptr<RealTimeScheduler> createRealTimeScheduler(const TaskSchedulerConfig &config,
                                                           std::shared_ptr<spdlog::logger> logger) {
    try {
        auto scheduler = std::make_unique<RealTimeScheduler>(logger);
        ErrorCode result = scheduler->configure(config);
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("Failed to configure real-time scheduler: {}", static_cast<int>(result));
            return nullptr;
        }
        RADAR_INFO("Created real-time scheduler");
        return scheduler;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception creating real-time scheduler: {}", e.what());
        return nullptr;
    }
}

std::unique_ptr<TaskScheduler> createScheduler(SchedulerType schedulerType, const TaskSchedulerConfig &config,
                                               std::shared_ptr<spdlog::logger> logger) {
    switch (schedulerType) {
        case SchedulerType::THREAD_POOL:
            return createThreadPoolScheduler(config, logger);
        case SchedulerType::REAL_TIME:
            return createRealTimeScheduler(config, logger);
        case SchedulerType::LOAD_BALANCE:
            RADAR_WARN("Load balance scheduler not implemented yet, using thread pool");
            return createThreadPoolScheduler(config, logger);
        case SchedulerType::DISTRIBUTED:
            RADAR_WARN("Distributed scheduler not implemented yet, using thread pool");
            return createThreadPoolScheduler(config, logger);
        default:
            RADAR_ERROR("Unknown scheduler type: {}", static_cast<int>(schedulerType));
            return nullptr;
    }
}

bool isSchedulerTypeAvailable(SchedulerType schedulerType) {
    switch (schedulerType) {
        case SchedulerType::THREAD_POOL:
        case SchedulerType::REAL_TIME:
            return true;
        case SchedulerType::LOAD_BALANCE:
        case SchedulerType::DISTRIBUTED:
            return false;  // 预留功能，暂未实现
        default:
            return false;
    }
}

}  // namespace TaskSchedulerFactory

}  // namespace radar
