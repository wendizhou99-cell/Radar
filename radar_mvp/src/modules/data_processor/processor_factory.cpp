/**
 * @file processor_factory.cpp
 * @brief 数据处理器工厂实现
 *
 * 实现了数据处理器的工厂类DataProcessorFactory，提供统一的
 * 创建接口，支持CPU、GPU、混合处理器的创建和管理。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "modules/data_processor.h"
#include "common/logger.h"

// CUDA相关头文件（条件编译）
#ifdef CUDA_ENABLED
#include <cuda_runtime.h>
#endif

namespace radar
{

    //==============================================================================
    // DataProcessorFactory 实现
    //==============================================================================

    namespace DataProcessorFactory
    {

        /**
         * @brief 创建CPU数据处理器实例
         * @param config 处理器配置参数
         * @param logger 日志记录器，可选参数
         * @return std::unique_ptr<CPUDataProcessor> CPU处理器智能指针
         * @retval nullptr 创建或配置失败
         * @retval 有效指针 创建成功的CPU处理器实例
         *
         * @note 创建的处理器已完成配置，可以直接使用
         * @note 如果配置失败，会记录错误日志并返回nullptr
         */
        std::unique_ptr<CPUDataProcessor> createCPUProcessor(
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger)
        {
            try
            {
                auto processor = std::make_unique<CPUDataProcessor>(logger);

                ErrorCode configResult = processor->configure(config);
                if (configResult != SystemErrors::SUCCESS)
                {
                    if (logger)
                    {
                        MODULE_ERROR(DataProcessorFactory, "Failed to configure CPU processor: {}",
                                     configResult);
                    }
                    return nullptr;
                }

                return processor;
            }
            catch (const std::exception &e)
            {
                if (logger)
                {
                    MODULE_ERROR(DataProcessorFactory, "Exception creating CPU processor: {}", e.what());
                }
                return nullptr;
            }
        }

        /**
         * @brief 创建GPU数据处理器实例
         * @param config 处理器配置参数
         * @param logger 日志记录器，可选参数
         * @return std::unique_ptr<GPUDataProcessor> GPU处理器智能指针
         * @retval nullptr 创建或配置失败
         * @retval 有效指针 创建成功的GPU处理器实例
         *
         * @note 创建的处理器已完成配置，可以直接使用
         * @note 如果GPU不可用，处理器会自动降级到CPU处理模式
         */
        std::unique_ptr<GPUDataProcessor> createGPUProcessor(
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger)
        {
            try
            {
                auto processor = std::make_unique<GPUDataProcessor>(logger);

                ErrorCode configResult = processor->configure(config);
                if (configResult != SystemErrors::SUCCESS)
                {
                    if (logger)
                    {
                        MODULE_ERROR(DataProcessorFactory, "Failed to configure GPU processor: {}",
                                     configResult);
                    }
                    return nullptr;
                }

                return processor;
            }
            catch (const std::exception &e)
            {
                if (logger)
                {
                    MODULE_ERROR(DataProcessorFactory, "Exception creating GPU processor: {}", e.what());
                }
                return nullptr;
            }
        }

        /**
         * @brief 创建指定类型的数据处理器
         * @param processorType 处理器类型（CPU、GPU或混合）
         * @param config 处理器配置参数
         * @param logger 日志记录器，可选参数
         * @return std::unique_ptr<DataProcessor> 数据处理器基类智能指针
         * @retval nullptr 创建失败
         * @retval 有效指针 创建成功的处理器实例
         *
         * @note 混合处理器会自动选择最佳可用硬件
         * @note 如果指定的硬件不可用，会自动降级处理
         */
        std::unique_ptr<DataProcessor> createProcessor(
            ProcessorType processorType,
            const DataProcessorConfig &config,
            std::shared_ptr<spdlog::logger> logger)
        {
            switch (processorType)
            {
            case ProcessorType::CPU_PROCESSOR:
                return createCPUProcessor(config, logger);

            case ProcessorType::GPU_PROCESSOR:
                return createGPUProcessor(config, logger);

            case ProcessorType::HYBRID_PROCESSOR:
                // 尝试创建GPU处理器，失败则降级到CPU
                if (isProcessorTypeAvailable(ProcessorType::GPU_PROCESSOR))
                {
                    return createGPUProcessor(config, logger);
                }
                else
                {
                    if (logger)
                    {
                        MODULE_WARN(DataProcessorFactory, "GPU not available, using CPU processor");
                    }
                    return createCPUProcessor(config, logger);
                }

            default:
                if (logger)
                {
                    MODULE_ERROR(DataProcessorFactory, "Unknown processor type: {}",
                                 static_cast<int>(processorType));
                }
                return nullptr;
            }
        }

        /**
         * @brief 检查指定类型的处理器是否可用
         * @param processorType 要检查的处理器类型
         * @return bool 处理器是否可用
         * @retval true 处理器硬件可用且支持
         * @retval false 处理器硬件不可用或不支持
         *
         * @note CPU处理器总是可用的
         * @note GPU处理器需要检查CUDA设备可用性
         * @note 混合处理器的可用性等同于GPU处理器
         */
        bool isProcessorTypeAvailable(ProcessorType processorType)
        {
            switch (processorType)
            {
            case ProcessorType::CPU_PROCESSOR:
                return true; // CPU处理器总是可用，无需额外硬件支持

            case ProcessorType::GPU_PROCESSOR:
            case ProcessorType::HYBRID_PROCESSOR:
#ifdef CUDA_ENABLED
                // 检查CUDA运行时环境和设备可用性
                int deviceCount = 0;
                cudaError_t result = cudaGetDeviceCount(&deviceCount);
                return (result == cudaSuccess && deviceCount > 0);
#else
                return false;
#endif

            default:
                return false;
            }
        }

    } // namespace DataProcessorFactory

} // namespace radar
