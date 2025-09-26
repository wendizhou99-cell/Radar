/**
 * @file display_controller.h
 * @brief 雷达显示控制器模块统一入口文件
 *
 * 本文件作为显示控制器模块的统一入口，包含所有必要的模块化接口。
 * 提供向后兼容性同时支持模块化的低耦合设计。
 *
 * 模块化结构：
 * - display_controller_base.h: 基础接口和抽象类
 * - display_controller_statistics.h: 统计信息和性能监控
 * - display_controller_implementations.h: 具体实现类
 * - display_controller_factory.h: 工厂模式和管理器
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see DisplayControllerBase
 * @see DisplayControllerFactory
 */

#pragma once

// 核心基础设施
#include "../../common/error_codes.h"
#include "../../common/interfaces.h"
#include "../../common/types.h"

// 模块化接口文件
#include "display_controller/display_controller_base.h"
#include "display_controller/display_controller_factory.h"
#include "display_controller/display_controller_implementations.h"
#include "display_controller/display_controller_statistics.h"

namespace radar {
// 为了向后兼容，将模块化命名空间中的类型别名到主命名空间
using DisplayController = modules::DisplayControllerBase;
using ConsoleDisplayController = modules::ConsoleDisplayController;
using FileDisplayController = modules::FileDisplayController;
using NetworkDisplayController = modules::NetworkDisplayController;
using HybridDisplayController = modules::HybridDisplayController;
using DisplayStatistics = modules::DisplayStatistics;
using DisplayControllerType = modules::DisplayControllerType;
using DisplayControllerFactory = modules::DisplayControllerFactory;

}  // namespace radar
