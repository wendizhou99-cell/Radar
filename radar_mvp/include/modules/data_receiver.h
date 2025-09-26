/**
 * @file data_receiver.h
 * @brief 雷达数据接收模块统一入口文件
 *
 * 本文件作为数据接收模块的统一入口，包含所有必要的模块化接口。
 * 提供向后兼容性同时支持模块化的低耦合设计。
 *
 * 模块化结构：
 * - data_receiver_base.h: 基础接口和抽象类
 * - data_receiver_statistics.h: 统计信息和性能监控
 * - data_receiver_implementations.h: 具体实现类
 * - data_receiver_factory.h: 工厂模式和管理器
 *
 * @author Klein
 * @version 2.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see DataReceiver
 * @see DataReceiverFactory
 */

#pragma once

// 模块化接口文件
#include "data_receiver/data_receiver_base.h"
#include "data_receiver/data_receiver_factory.h"
#include "data_receiver/data_receiver_implementations.h"
#include "data_receiver/data_receiver_statistics.h"

namespace radar {
// 为了向后兼容，将模块化命名空间中的类型别名到主命名空间
using DataReceiver = modules::DataReceiver;
using UDPDataReceiver = modules::UDPDataReceiver;
using FileDataReceiver = modules::FileDataReceiver;
using HardwareDataReceiver = modules::HardwareDataReceiver;
using SimulationDataReceiver = modules::SimulationDataReceiver;
using ReceptionStatistics = modules::ReceptionStatistics;
using PerformanceMonitor = modules::PerformanceMonitor;
using StatisticsManager = modules::StatisticsManager;
using ReceiverManager = modules::ReceiverManager;

// 工厂命名空间别名
namespace DataReceiverFactory = modules::DataReceiverFactory;

}  // namespace radar
