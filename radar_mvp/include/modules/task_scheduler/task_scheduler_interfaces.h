/**
 * @file task_scheduler_interfaces.h
 * @brief 任务调度器内部接口定义
 *
 * 定义任务调度器模块内部使用的接口和抽象类。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#pragma once

#include "task_scheduler_types.h"
#include <memory>

namespace radar
{

    /**
     * @brief 任务队列接口
     *
     * 定义了任务队列的基本操作，支持不同的调度策略。
     */
    class TaskQueue
    {
    public:
        /**
         * @brief 析构函数
         */
        virtual ~TaskQueue() = default;

        /**
         * @brief 添加任务到队列
         * @param task 任务对象
         * @return 操作结果错误码
         */
        virtual ErrorCode enqueue(const ScheduledTaskPtr &task) = 0;

        /**
         * @brief 从队列取出任务
         * @param task 输出参数，任务对象
         * @param timeoutMs 超时时间（毫秒）
         * @return 操作结果错误码
         */
        virtual ErrorCode dequeue(ScheduledTaskPtr &task, uint32_t timeoutMs = 1000) = 0;

        /**
         * @brief 获取队列大小
         * @return 队列中的任务数量
         */
        virtual size_t size() const = 0;

        /**
         * @brief 检查队列是否为空
         * @return 队列是否为空
         */
        virtual bool empty() const = 0;

        /**
         * @brief 清空队列
         */
        virtual void clear() = 0;
    };

} // namespace radar
