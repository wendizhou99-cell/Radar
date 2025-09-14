/**
 * @file error_codes.cpp
 * @brief 错误码描述信息的实现
 *
 * 实现了error_codes.h中声明的错误描述和错误级别查询函数。
 * 提供了完整的错误信息映射表，便于调试和日志记录。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include "common/error_codes.h"
#include <unordered_map>

namespace radar
{

    //==============================================================================
    // 错误描述映射表
    //==============================================================================

    /// 错误码到描述信息的映射表
    static const std::unordered_map<ErrorCode, const char *> errorDescriptions = {
        // 系统级错误 (0x0000 - 0x0FFF)
        {SystemErrors::SUCCESS, "操作成功"},
        {SystemErrors::UNKNOWN_ERROR, "未知错误"},
        {SystemErrors::INVALID_PARAMETER, "无效参数"},
        {SystemErrors::INSUFFICIENT_MEMORY, "内存不足"},
        {SystemErrors::RESOURCE_UNAVAILABLE, "资源不可用"},
        {SystemErrors::OPERATION_TIMEOUT, "操作超时"},
        {SystemErrors::INITIALIZATION_FAILED, "初始化失败"},
        {SystemErrors::SHUTDOWN_FAILED, "关闭失败"},
        {SystemErrors::CONFIGURATION_ERROR, "配置错误"},
        {SystemErrors::PERMISSION_DENIED, "权限拒绝"},

        // 数据接收模块错误 (0x1000 - 0x1FFF)
        {DataReceiverErrors::RECEIVER_NOT_READY, "数据接收器未就绪"},
        {DataReceiverErrors::DATA_SOURCE_ERROR, "数据源错误"},
        {DataReceiverErrors::PACKET_CORRUPTION, "数据包损坏"},
        {DataReceiverErrors::BUFFER_OVERFLOW, "接收缓冲区溢出"},
        {DataReceiverErrors::SEQUENCE_ERROR, "数据包序列号错误"},
        {DataReceiverErrors::TIMESTAMP_ERROR, "时间戳错误"},
        {DataReceiverErrors::CHANNEL_MISMATCH, "通道配置不匹配"},
        {DataReceiverErrors::SAMPLING_RATE_ERROR, "采样率配置错误"},
        {DataReceiverErrors::HARDWARE_FAILURE, "硬件故障"},
        {DataReceiverErrors::SIMULATION_ERROR, "模拟数据生成错误"},

        // 数据处理模块错误 (0x2000 - 0x2FFF)
        {DataProcessorErrors::PROCESSOR_NOT_READY, "数据处理器未就绪"},
        {DataProcessorErrors::INVALID_INPUT_DATA, "输入数据格式无效"},
        {DataProcessorErrors::PROCESSING_FAILED, "数据处理失败"},
        {DataProcessorErrors::ALGORITHM_ERROR, "处理算法执行错误"},
        {DataProcessorErrors::CUDA_ERROR, "CUDA运行时错误"},
        {DataProcessorErrors::GPU_MEMORY_ERROR, "GPU内存分配或访问错误"},
        {DataProcessorErrors::FFT_ERROR, "快速傅里叶变换计算错误"},
        {DataProcessorErrors::BEAMFORMING_ERROR, "波束形成算法错误"},
        {DataProcessorErrors::CALIBRATION_ERROR, "系统校准错误"},
        {DataProcessorErrors::PERFORMANCE_DEGRADED, "处理性能严重下降"},

        // 任务调度模块错误 (0x3000 - 0x3FFF)
        {TaskSchedulerErrors::SCHEDULER_NOT_READY, "任务调度器未就绪"},
        {TaskSchedulerErrors::TASK_QUEUE_FULL, "任务队列已满"},
        {TaskSchedulerErrors::TASK_EXECUTION_FAILED, "任务执行失败"},
        {TaskSchedulerErrors::THREAD_POOL_ERROR, "线程池管理错误"},
        {TaskSchedulerErrors::DEADLOCK_DETECTED, "检测到死锁情况"},
        {TaskSchedulerErrors::PRIORITY_INVERSION, "任务优先级反转"},
        {TaskSchedulerErrors::RESOURCE_CONTENTION, "系统资源竞争"},
        {TaskSchedulerErrors::SCHEDULING_ERROR, "任务调度策略错误"},
        {TaskSchedulerErrors::TASK_TIMEOUT, "任务执行超时"},
        {TaskSchedulerErrors::LOAD_BALANCING_ERROR, "负载均衡策略错误"},

        // 显示控制模块错误 (0x4000 - 0x4FFF)
        {DisplayControllerErrors::DISPLAY_NOT_READY, "显示控制器未就绪"},
        {DisplayControllerErrors::RENDER_ERROR, "渲染过程错误"},
        {DisplayControllerErrors::OUTPUT_FORMAT_ERROR, "输出格式不支持"},
        {DisplayControllerErrors::FILE_WRITE_ERROR, "文件写入错误"},
        {DisplayControllerErrors::CONSOLE_OUTPUT_ERROR, "控制台输出错误"},
        {DisplayControllerErrors::GRAPHICS_ERROR, "图形系统错误"},
        {DisplayControllerErrors::DISPLAY_BUFFER_ERROR, "显示缓冲区错误"},
        {DisplayControllerErrors::REFRESH_RATE_ERROR, "刷新率配置错误"},
        {DisplayControllerErrors::COLOR_SPACE_ERROR, "色彩空间转换错误"}};

    /// 错误码到错误级别的映射表
    static const std::unordered_map<ErrorCode, ErrorLevel> errorLevels = {
        // 系统级错误级别
        {SystemErrors::SUCCESS, ErrorLevel::INFO},
        {SystemErrors::UNKNOWN_ERROR, ErrorLevel::ERROR},
        {SystemErrors::INVALID_PARAMETER, ErrorLevel::WARNING},
        {SystemErrors::INSUFFICIENT_MEMORY, ErrorLevel::CRITICAL},
        {SystemErrors::RESOURCE_UNAVAILABLE, ErrorLevel::ERROR},
        {SystemErrors::OPERATION_TIMEOUT, ErrorLevel::WARNING},
        {SystemErrors::INITIALIZATION_FAILED, ErrorLevel::CRITICAL},
        {SystemErrors::SHUTDOWN_FAILED, ErrorLevel::ERROR},
        {SystemErrors::CONFIGURATION_ERROR, ErrorLevel::ERROR},
        {SystemErrors::PERMISSION_DENIED, ErrorLevel::ERROR},

        // 数据接收模块错误级别
        {DataReceiverErrors::RECEIVER_NOT_READY, ErrorLevel::WARNING},
        {DataReceiverErrors::DATA_SOURCE_ERROR, ErrorLevel::ERROR},
        {DataReceiverErrors::PACKET_CORRUPTION, ErrorLevel::WARNING},
        {DataReceiverErrors::BUFFER_OVERFLOW, ErrorLevel::WARNING},
        {DataReceiverErrors::SEQUENCE_ERROR, ErrorLevel::WARNING},
        {DataReceiverErrors::TIMESTAMP_ERROR, ErrorLevel::WARNING},
        {DataReceiverErrors::CHANNEL_MISMATCH, ErrorLevel::ERROR},
        {DataReceiverErrors::SAMPLING_RATE_ERROR, ErrorLevel::ERROR},
        {DataReceiverErrors::HARDWARE_FAILURE, ErrorLevel::CRITICAL},
        {DataReceiverErrors::SIMULATION_ERROR, ErrorLevel::ERROR},

        // 数据处理模块错误级别
        {DataProcessorErrors::PROCESSOR_NOT_READY, ErrorLevel::WARNING},
        {DataProcessorErrors::INVALID_INPUT_DATA, ErrorLevel::WARNING},
        {DataProcessorErrors::PROCESSING_FAILED, ErrorLevel::ERROR},
        {DataProcessorErrors::ALGORITHM_ERROR, ErrorLevel::ERROR},
        {DataProcessorErrors::CUDA_ERROR, ErrorLevel::CRITICAL},
        {DataProcessorErrors::GPU_MEMORY_ERROR, ErrorLevel::CRITICAL},
        {DataProcessorErrors::FFT_ERROR, ErrorLevel::ERROR},
        {DataProcessorErrors::BEAMFORMING_ERROR, ErrorLevel::ERROR},
        {DataProcessorErrors::CALIBRATION_ERROR, ErrorLevel::ERROR},
        {DataProcessorErrors::PERFORMANCE_DEGRADED, ErrorLevel::WARNING},

        // 任务调度模块错误级别
        {TaskSchedulerErrors::SCHEDULER_NOT_READY, ErrorLevel::WARNING},
        {TaskSchedulerErrors::TASK_QUEUE_FULL, ErrorLevel::WARNING},
        {TaskSchedulerErrors::TASK_EXECUTION_FAILED, ErrorLevel::ERROR},
        {TaskSchedulerErrors::THREAD_POOL_ERROR, ErrorLevel::ERROR},
        {TaskSchedulerErrors::DEADLOCK_DETECTED, ErrorLevel::CRITICAL},
        {TaskSchedulerErrors::PRIORITY_INVERSION, ErrorLevel::WARNING},
        {TaskSchedulerErrors::RESOURCE_CONTENTION, ErrorLevel::WARNING},
        {TaskSchedulerErrors::SCHEDULING_ERROR, ErrorLevel::ERROR},
        {TaskSchedulerErrors::TASK_TIMEOUT, ErrorLevel::WARNING},
        {TaskSchedulerErrors::LOAD_BALANCING_ERROR, ErrorLevel::WARNING},

        // 显示控制模块错误级别
        {DisplayControllerErrors::DISPLAY_NOT_READY, ErrorLevel::WARNING},
        {DisplayControllerErrors::RENDER_ERROR, ErrorLevel::ERROR},
        {DisplayControllerErrors::OUTPUT_FORMAT_ERROR, ErrorLevel::WARNING},
        {DisplayControllerErrors::FILE_WRITE_ERROR, ErrorLevel::ERROR},
        {DisplayControllerErrors::CONSOLE_OUTPUT_ERROR, ErrorLevel::WARNING},
        {DisplayControllerErrors::GRAPHICS_ERROR, ErrorLevel::ERROR},
        {DisplayControllerErrors::DISPLAY_BUFFER_ERROR, ErrorLevel::WARNING},
        {DisplayControllerErrors::REFRESH_RATE_ERROR, ErrorLevel::WARNING},
        {DisplayControllerErrors::COLOR_SPACE_ERROR, ErrorLevel::WARNING}};

    //==============================================================================
    // 实现函数
    //==============================================================================

    const char *getErrorDescription(ErrorCode errorCode)
    {
        auto it = errorDescriptions.find(errorCode);
        if (it != errorDescriptions.end())
        {
            return it->second;
        }
        return "未知错误码";
    }

    ErrorLevel getErrorLevel(ErrorCode errorCode)
    {
        auto it = errorLevels.find(errorCode);
        if (it != errorLevels.end())
        {
            return it->second;
        }

        // 根据错误码范围推断错误级别
        if (isSystemError(errorCode))
        {
            return ErrorLevel::ERROR;
        }
        else
        {
            return ErrorLevel::WARNING;
        }
    }

} // namespace radar
