/**
 * @file data_receiver_factory.h
 * @brief 雷达数据接收器工厂模式接口定义
 *
 * 本文件定义了数据接收器的工厂创建接口，提供统一的
 * 创建和管理接口，支持多种接收器类型的动态创建。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see DataReceiverFactory
 * @see ReceiverType
 */

#pragma once

#include "data_receiver_implementations.h"
#include "common/logger.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>

namespace radar
{
    namespace modules
    {
        /**
         * @brief 数据接收器工厂命名空间
         */
        namespace DataReceiverFactory
        {
            /**
             * @brief 数据接收器类型枚举
             */
            enum class ReceiverType : uint8_t
            {
                UDP_RECEIVER = 0,    ///< UDP 网络接收器
                FILE_RECEIVER,       ///< 文件数据接收器
                HARDWARE_RECEIVER,   ///< 硬件数据接收器
                SIMULATION_RECEIVER, ///< 模拟数据接收器
                AUTO_SELECT          ///< 自动选择（优先硬件）
            };

            /**
             * @brief 自动创建合适的数据接收器
             *
             * 根据配置参数和系统环境自动选择最合适的接收器类型。
             *
             * @param receiverType 接收器类型
             * @param config 接收器配置
             * @param logger 日志记录器实例
             * @return 数据接收器智能指针，创建失败时返回 nullptr
             */
            std::unique_ptr<DataReceiver> createReceiver(
                ReceiverType receiverType,
                const DataReceiverConfig &config,
                std::shared_ptr<spdlog::logger> logger = nullptr);

            /**
             * @brief 检查指定类型的接收器是否可用
             *
             * @param receiverType 接收器类型
             * @return true 如果接收器类型可用，false 否则
             */
            bool isReceiverTypeAvailable(ReceiverType receiverType);

            /**
             * @brief 获取可用的接收器类型列表
             *
             * @return 可用接收器类型的向量
             */
            std::vector<ReceiverType> getAvailableReceiverTypes();

            /**
             * @brief 获取接收器类型的描述信息
             *
             * @param receiverType 接收器类型
             * @return 类型描述字符串
             */
            std::string getReceiverTypeDescription(ReceiverType receiverType);

            /**
             * @brief 获取接收器类型的名称
             *
             * @param receiverType 接收器类型
             * @return 类型名称字符串
             */
            std::string getReceiverTypeName(ReceiverType receiverType);

            /**
             * @brief 从字符串解析接收器类型
             *
             * @param typeName 类型名称字符串
             * @return 接收器类型，解析失败时返回 AUTO_SELECT
             */
            ReceiverType parseReceiverType(const std::string &typeName);

            /**
             * @brief 验证接收器配置
             *
             * 检查给定的配置是否适用于指定的接收器类型。
             *
             * @param receiverType 接收器类型
             * @param config 配置参数
             * @return true 如果配置有效，false 否则
             */
            bool validateReceiverConfig(ReceiverType receiverType,
                                        const DataReceiverConfig &config);

            /**
             * @brief 获取接收器类型的默认配置
             *
             * @param receiverType 接收器类型
             * @return 默认配置参数
             */
            DataReceiverConfig getDefaultConfig(ReceiverType receiverType);

        } // namespace DataReceiverFactory

        /**
         * @brief 接收器管理器类
         *
         * 提供接收器实例的统一管理，支持生命周期管理、
         * 状态监控和性能统计等功能。
         */
        class ReceiverManager
        {
        public:
            /**
             * @brief 构造函数
             */
            ReceiverManager();

            /**
             * @brief 析构函数
             */
            ~ReceiverManager();

            /**
             * @brief 注册接收器实例
             *
             * @param name 接收器名称
             * @param receiver 接收器实例
             * @return true 如果注册成功，false 否则
             */
            bool registerReceiver(const std::string &name,
                                  std::unique_ptr<DataReceiver> receiver);

            /**
             * @brief 注销接收器实例
             *
             * @param name 接收器名称
             * @return true 如果注销成功，false 否则
             */
            bool unregisterReceiver(const std::string &name);

            /**
             * @brief 获取接收器实例
             *
             * @param name 接收器名称
             * @return 接收器指针，不存在时返回 nullptr
             */
            DataReceiver *getReceiver(const std::string &name);

            /**
             * @brief 启动所有接收器
             *
             * @return 启动成功的接收器数量
             */
            size_t startAllReceivers();

            /**
             * @brief 停止所有接收器
             *
             * @return 停止成功的接收器数量
             */
            size_t stopAllReceivers();

            /**
             * @brief 获取所有接收器的状态
             *
             * @return 状态映射表（名称 -> 状态）
             */
            std::map<std::string, ModuleState> getAllReceiverStates() const;

            /**
             * @brief 生成接收器状态报告
             *
             * @return 格式化的状态报告字符串
             */
            std::string generateStatusReport() const;

        private:
            std::map<std::string, std::unique_ptr<DataReceiver>> receivers_;
            mutable std::mutex managerMutex_;
        };

    } // namespace modules
} // namespace radar
