/**
 * @file main.cpp
 * @brief 雷达MVP系统主程序入口
 *
 * 系统的主要入口点，负责初始化各个模块并启动系统
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include <iostream>

#include "application/radar_application.h"
#include "common/config_manager.h"
#include "common/error_codes.h"
#include "common/logger.h"
#include "common/service_provider.h"  // 引入服务提供者

using namespace radar::common;
using namespace radar::application;
using namespace radar;

int main() {
    std::cout << "=== 雷达MVP系统启动 ===" << std::endl;

    try {
        // 1. 创建服务提供者
        auto serviceProvider = std::make_shared<ServiceProvider>();
        RADAR_INFO("服务提供者创建成功");

        // 2. 初始化并注册基础服务
        // 初始化日志系统
        auto loggerManager = std::make_shared<LoggerManager>();
        LoggerConfig logConfig;
        logConfig.console.enabled = true;
        logConfig.console.level = LogLevel::INFO;
        auto result = loggerManager->initialize(logConfig);
        if (result != SystemErrors::SUCCESS) {
            std::cerr << "日志系统初始化失败" << std::endl;
            return -1;
        }
        serviceProvider->registerService(loggerManager);
        RADAR_INFO("日志服务注册成功");

        // 加载配置文件
        auto configManager = std::make_shared<ConfigManager>();
        result = configManager->loadFromFile("configs/config.yaml");
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("配置文件加载失败");
            return -1;
        }
        serviceProvider->registerService(configManager);
        RADAR_INFO("配置服务注册成功");

        RADAR_INFO("核心基础服务初始化并注册完成");

        // 3. 创建并初始化应用程序，注入服务提供者
        RadarApplication app(serviceProvider);
        result = app.initialize();
        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("应用程序初始化失败");
            return -1;
        }

        RADAR_INFO("雷达应用程序初始化完成");
        RADAR_INFO("系统启动成功！");

        // 简单的运行循环
        std::cout << "按回车键退出..." << std::endl;
        std::cin.get();

        // 清理资源
        app.shutdown();
        loggerManager->shutdown();

        RADAR_INFO("系统已正常关闭");
    } catch (const std::exception &e) {
        std::cerr << "系统运行时发生异常: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
