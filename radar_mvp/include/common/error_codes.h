/**
 * @file error_codes.h
 * @brief 雷达MVP系统全局错误码定义
 *
 * 定义了整个系统的统一错误处理机制，包括错误码枚举、错误信息描述、
 * 异常类定义和错误处理工具函数。采用分层错误码设计，便于定位问题。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see types.h
 * @see interfaces.h
 */

#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

namespace radar {

//==============================================================================
// 错误码类型定义
//==============================================================================

/// 错误码基础类型
using ErrorCode = uint32_t;

/// 错误级别枚举
enum class ErrorLevel : uint8_t {
    INFO = 0,  ///< 信息级别
    WARNING,   ///< 警告级别
    ERROR,     ///< 错误级别
    CRITICAL,  ///< 关键错误级别
    FATAL      ///< 致命错误级别
};

//==============================================================================
// 分层错误码定义
//==============================================================================

/**
 * @brief 系统级错误码 (0x0000 - 0x0FFF)
 * @details 影响整个系统运行的基础错误
 */
namespace SystemErrors {
constexpr ErrorCode SUCCESS = 0x0000;                ///< 操作成功
constexpr ErrorCode UNKNOWN_ERROR = 0x0001;          ///< 未知错误
constexpr ErrorCode INVALID_PARAMETER = 0x0002;      ///< 无效参数
constexpr ErrorCode INSUFFICIENT_MEMORY = 0x0003;    ///< 内存不足
constexpr ErrorCode RESOURCE_UNAVAILABLE = 0x0004;   ///< 资源不可用
constexpr ErrorCode OPERATION_TIMEOUT = 0x0005;      ///< 操作超时
constexpr ErrorCode INITIALIZATION_FAILED = 0x0006;  ///< 初始化失败
constexpr ErrorCode SHUTDOWN_FAILED = 0x0007;        ///< 关闭失败
constexpr ErrorCode CONFIGURATION_ERROR = 0x0008;    ///< 配置错误
constexpr ErrorCode PERMISSION_DENIED = 0x0009;      ///< 权限拒绝
}  // namespace SystemErrors

/**
 * @brief 数据接收模块错误码 (0x1000 - 0x1FFF)
 * @details 数据接收和输入处理相关错误
 */
namespace DataReceiverErrors {
constexpr ErrorCode BASE = 0x1000;                      ///< 模块错误码基址
constexpr ErrorCode RECEIVER_NOT_READY = 0x1001;        ///< 接收器未就绪
constexpr ErrorCode RECEIVER_ALREADY_RUNNING = 0x1002;  ///< 接收器已在运行
constexpr ErrorCode DATA_SOURCE_ERROR = 0x1003;         ///< 数据源错误
constexpr ErrorCode PACKET_CORRUPTION = 0x1004;         ///< 数据包损坏
constexpr ErrorCode BUFFER_OVERFLOW = 0x1005;           ///< 缓冲区溢出
constexpr ErrorCode SEQUENCE_ERROR = 0x1006;            ///< 序列号错误
constexpr ErrorCode TIMESTAMP_ERROR = 0x1007;           ///< 时间戳错误
constexpr ErrorCode CHANNEL_MISMATCH = 0x1008;          ///< 通道不匹配
constexpr ErrorCode SAMPLING_RATE_ERROR = 0x1009;       ///< 采样率错误
constexpr ErrorCode HARDWARE_FAILURE = 0x100A;          ///< 硬件故障
constexpr ErrorCode SIMULATION_ERROR = 0x100B;          ///< 模拟数据错误
}  // namespace DataReceiverErrors

/**
 * @brief 数据处理模块错误码 (0x2000 - 0x2FFF)
 * @details 信号处理和算法计算相关错误
 */
namespace DataProcessorErrors {
constexpr ErrorCode BASE = 0x2000;                  ///< 模块错误码基址
constexpr ErrorCode PROCESSOR_NOT_READY = 0x2001;   ///< 处理器未就绪
constexpr ErrorCode INVALID_INPUT_DATA = 0x2002;    ///< 无效输入数据
constexpr ErrorCode PROCESSING_FAILED = 0x2003;     ///< 处理失败
constexpr ErrorCode ALGORITHM_ERROR = 0x2004;       ///< 算法错误
constexpr ErrorCode CUDA_ERROR = 0x2005;            ///< CUDA错误
constexpr ErrorCode GPU_MEMORY_ERROR = 0x2006;      ///< GPU内存错误
constexpr ErrorCode FFT_ERROR = 0x2007;             ///< FFT计算错误
constexpr ErrorCode BEAMFORMING_ERROR = 0x2008;     ///< 波束形成错误
constexpr ErrorCode CALIBRATION_ERROR = 0x2009;     ///< 校准错误
constexpr ErrorCode PERFORMANCE_DEGRADED = 0x200A;  ///< 性能下降
}  // namespace DataProcessorErrors

/**
 * @brief 任务调度模块错误码 (0x3000 - 0x3FFF)
 * @details 任务管理和线程调度相关错误
 */
namespace TaskSchedulerErrors {
constexpr ErrorCode BASE = 0x3000;                   ///< 模块错误码基址
constexpr ErrorCode SCHEDULER_NOT_READY = 0x3001;    ///< 调度器未就绪
constexpr ErrorCode TASK_QUEUE_FULL = 0x3002;        ///< 任务队列已满
constexpr ErrorCode TASK_EXECUTION_FAILED = 0x3003;  ///< 任务执行失败
constexpr ErrorCode THREAD_POOL_ERROR = 0x3004;      ///< 线程池错误
constexpr ErrorCode DEADLOCK_DETECTED = 0x3005;      ///< 检测到死锁
constexpr ErrorCode PRIORITY_INVERSION = 0x3006;     ///< 优先级反转
constexpr ErrorCode RESOURCE_CONTENTION = 0x3007;    ///< 资源竞争
constexpr ErrorCode SCHEDULING_ERROR = 0x3008;       ///< 调度错误
constexpr ErrorCode TASK_TIMEOUT = 0x3009;           ///< 任务超时
constexpr ErrorCode LOAD_BALANCING_ERROR = 0x300A;   ///< 负载均衡错误
}  // namespace TaskSchedulerErrors

/**
 * @brief 显示控制模块错误码 (0x4000 - 0x4FFF)
 * @details 显示输出和用户界面相关错误
 */
namespace DisplayControllerErrors {
constexpr ErrorCode BASE = 0x4000;                  ///< 模块错误码基址
constexpr ErrorCode DISPLAY_NOT_READY = 0x4001;     ///< 显示器未就绪
constexpr ErrorCode RENDER_ERROR = 0x4002;          ///< 渲染错误
constexpr ErrorCode OUTPUT_FORMAT_ERROR = 0x4003;   ///< 输出格式错误
constexpr ErrorCode FILE_WRITE_ERROR = 0x4004;      ///< 文件写入错误
constexpr ErrorCode CONSOLE_OUTPUT_ERROR = 0x4005;  ///< 控制台输出错误
constexpr ErrorCode GRAPHICS_ERROR = 0x4006;        ///< 图形错误
constexpr ErrorCode DISPLAY_BUFFER_ERROR = 0x4007;  ///< 显示缓冲区错误
constexpr ErrorCode REFRESH_RATE_ERROR = 0x4008;    ///< 刷新率错误
constexpr ErrorCode COLOR_SPACE_ERROR = 0x4009;     ///< 色彩空间错误
}  // namespace DisplayControllerErrors

//==============================================================================
// 错误信息描述
//==============================================================================

/**
 * @brief 获取错误码对应的描述信息
 * @param errorCode 错误码
 * @return 错误描述字符串
 */
const char *getErrorDescription(ErrorCode errorCode);

/**
 * @brief 获取错误级别
 * @param errorCode 错误码
 * @return 错误级别枚举
 */
ErrorLevel getErrorLevel(ErrorCode errorCode);

/**
 * @brief 检查错误码是否表示成功
 * @param errorCode 错误码
 * @return 是否成功
 */
inline bool isSuccess(ErrorCode errorCode) {
    return errorCode == SystemErrors::SUCCESS;
}

/**
 * @brief 检查是否为系统级错误
 * @param errorCode 错误码
 * @return 是否为系统级错误
 */
inline bool isSystemError(ErrorCode errorCode) {
    return (errorCode & 0xF000) == 0x0000;
}

/**
 * @brief 检查是否为模块级错误
 * @param errorCode 错误码
 * @return 是否为模块级错误
 */
inline bool isModuleError(ErrorCode errorCode) {
    return (errorCode & 0xF000) != 0x0000;
}

//==============================================================================
// 异常类定义
//==============================================================================

/**
 * @brief 雷达系统基础异常类
 * @details 所有雷达系统异常的基类，包含错误码和详细信息
 */
class RadarException : public std::exception {
  public:
    /**
     * @brief 构造函数
     * @param errorCode 错误码
     * @param message 自定义错误信息
     * @param fileName 发生错误的文件名
     * @param lineNumber 发生错误的行号
     */
    RadarException(ErrorCode errorCode, const std::string &message = "", const char *fileName = nullptr,
                   int lineNumber = 0)
        : errorCode_(errorCode), customMessage_(message), fileName_(fileName ? fileName : ""), lineNumber_(lineNumber) {
        std::ostringstream oss;
        oss << "RadarException [0x" << std::hex << errorCode_ << "]: " << getErrorDescription(errorCode_);

        if (!customMessage_.empty()) {
            oss << " - " << customMessage_;
        }

        if (!fileName_.empty()) {
            oss << " (at " << fileName_ << ":" << lineNumber_ << ")";
        }

        fullMessage_ = oss.str();
    }

    /**
     * @brief 获取异常信息
     * @return 异常描述字符串
     */
    const char *what() const noexcept override {
        return fullMessage_.c_str();
    }

    /**
     * @brief 获取错误码
     * @return 错误码
     */
    ErrorCode getErrorCode() const noexcept {
        return errorCode_;
    }

    /**
     * @brief 获取错误级别
     * @return 错误级别
     */
    ErrorLevel getErrorLevel() const noexcept {
        return radar::getErrorLevel(errorCode_);
    }

  private:
    ErrorCode errorCode_;        ///< 错误码
    std::string customMessage_;  ///< 自定义错误信息
    std::string fileName_;       ///< 文件名
    int lineNumber_;             ///< 行号
    std::string fullMessage_;    ///< 完整错误信息
};

/**
 * @brief 系统级异常类
 * @details 系统级别的异常，通常需要系统级处理
 */
class SystemException : public RadarException {
  public:
    SystemException(ErrorCode errorCode, const std::string &message = "", const char *fileName = nullptr,
                    int lineNumber = 0)
        : RadarException(errorCode, message, fileName, lineNumber) {}
};

/**
 * @brief 模块级异常类
 * @details 模块级别的异常，可以在模块内部处理
 */
class ModuleException : public RadarException {
  public:
    ModuleException(ErrorCode errorCode, const std::string &message = "", const char *fileName = nullptr,
                    int lineNumber = 0)
        : RadarException(errorCode, message, fileName, lineNumber) {}
};

//==============================================================================
// 错误处理宏定义
//==============================================================================

/// 抛出雷达异常的便捷宏
#define RADAR_THROW(errorCode, message) throw radar::RadarException(errorCode, message, __FILE__, __LINE__)

/// 抛出系统异常的便捷宏
#define SYSTEM_THROW(errorCode, message) throw radar::SystemException(errorCode, message, __FILE__, __LINE__)

/// 抛出模块异常的便捷宏
#define MODULE_THROW(errorCode, message) throw radar::ModuleException(errorCode, message, __FILE__, __LINE__)

/// 检查条件，如果失败则抛出异常
#define RADAR_ASSERT(condition, errorCode, message) \
    do {                                            \
        if (!(condition)) {                         \
            RADAR_THROW(errorCode, message);        \
        }                                           \
    } while (0)

/// 检查错误码，如果不是成功则抛出异常
#define RADAR_CHECK_ERROR(errorCode)                    \
    do {                                                \
        if (!radar::isSuccess(errorCode)) {             \
            RADAR_THROW(errorCode, "Operation failed"); \
        }                                               \
    } while (0)

}  // namespace radar
