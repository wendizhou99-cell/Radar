/**
 * @file task_scheduler.h
 * @brief 任务调度模块的统一接口定义
 *
 * 定义了雷达系统任务调度的抽象接口以及具体实现类。
 * 支持多种调度策略并实现 ITaskScheduler 接口。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 *
 * @see ITaskScheduler
 * @see TaskScheduler
 */

#pragma once

// 包含所有子模块头文件
#include "task_scheduler_factory.h"
#include "task_scheduler_implementations.h"
#include "task_scheduler_interfaces.h"
#include "task_scheduler_types.h"
