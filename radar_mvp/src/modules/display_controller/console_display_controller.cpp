/**
 * @file console_display_controller.cpp
 * @brief 控制台显示控制器实现
 *
 * 实现了ConsoleDisplayController类，提供控制台数据显示功能。
 * 支持彩色输出、表格格式化、实时数据显示等特性。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "modules/display_controller/display_controller_implementations.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace radar
{
    namespace modules
    {

        //==============================================================================
        // 构造函数和析构函数
        //==============================================================================

        ConsoleDisplayController::ConsoleDisplayController()
            : DisplayControllerBase("ConsoleDisplayController"),
              displayedLines_(0)
        {
            RADAR_INFO("ConsoleDisplayController 创建");
        }

        ConsoleDisplayController::~ConsoleDisplayController()
        {
            RADAR_INFO("ConsoleDisplayController 销毁");
        }

        //==============================================================================
        // DisplayControllerBase 虚函数实现
        //==============================================================================

        ErrorCode ConsoleDisplayController::initializeDisplay()
        {
            try
            {
                // 重置显示行数
                displayedLines_ = 0;

                // 检查控制台是否可用
                if (!std::cout.good())
                {
                    RADAR_ERROR("控制台输出流不可用");
                    return DisplayControllerErrors::DISPLAY_NOT_READY;
                }

                RADAR_INFO("ConsoleDisplayController 初始化成功");
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("ConsoleDisplayController 初始化异常: {}", e.what());
                return SystemErrors::INITIALIZATION_FAILED;
            }
        }

        ErrorCode ConsoleDisplayController::cleanupDisplay()
        {
            try
            {
                // 输出清理信息
                std::lock_guard<std::mutex> lock(outputMutex_);
                std::cout << "\n=== Console Display Controller Shutdown ===\n";
                std::cout << "Total lines displayed: " << displayedLines_ << "\n";
                std::cout << "===========================================\n"
                          << std::endl;

                // 刷新输出缓冲区
                std::cout.flush();

                RADAR_INFO("ConsoleDisplayController 清理完成");
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("ConsoleDisplayController 清理异常: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        ErrorCode ConsoleDisplayController::renderData(const DisplayData &data)
        {
            try
            {
                std::lock_guard<std::mutex> lock(outputMutex_);

                // 格式化输出内容
                std::string output = formatConsoleOutput(data);

                // 输出到控制台
                std::cout << output << std::endl;

                // 更新统计信息
                displayedLines_++;

                // 检查是否需要清理旧内容
                if (consoleConfig_.maxLines > 0 && displayedLines_ >= consoleConfig_.maxLines)
                {
                    // 在控制台中，我们通过换页来"清理"
                    std::cout << "\n"
                              << std::string(50, '=') << "\n";
                    std::cout << "Console buffer cleared (max lines reached)\n";
                    std::cout << std::string(50, '=') << "\n\n";
                    displayedLines_ = 0;
                }

                // 确保输出立即显示
                std::cout.flush();

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("ConsoleDisplayController 渲染异常: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        std::vector<radar::IDisplayController::DisplayFormat> ConsoleDisplayController::getSpecificSupportedFormats() const
        {
            return {
                radar::IDisplayController::DisplayFormat::CONSOLE_TEXT,
                radar::IDisplayController::DisplayFormat::CONSOLE_CHART};
        }

        ErrorCode ConsoleDisplayController::saveDisplayToFile(const std::string &filePath,
                                                              const DisplayData &data)
        {
            try
            {
                // 打开文件进行追加写入
                std::ofstream file(filePath, std::ios::app);
                if (!file.is_open())
                {
                    RADAR_ERROR("无法打开文件进行写入: {}", filePath);
                    return SystemErrors::RESOURCE_UNAVAILABLE;
                }

                // 格式化输出内容（不包含控制台特定格式）
                std::string output = formatConsoleOutput(data);

                // 移除ANSI颜色代码（如果有的话）
                // 这里可以添加颜色代码移除逻辑

                // 写入文件
                file << output << "\n";
                file.close();

                RADAR_INFO("控制台显示内容已保存到文件: {}", filePath);
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("保存到文件异常: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        //==============================================================================
        // 私有辅助方法
        //==============================================================================

        std::string ConsoleDisplayController::formatConsoleOutput(const DisplayData &data)
        {
            std::ostringstream oss;

            // 添加标题
            if (consoleConfig_.showHeaders && !data.metadata.title.empty())
            {
                oss << colorizeText("=== " + data.metadata.title + " ===", 36) << "\n"; // 青色
            }

            // 添加时间戳
            if (consoleConfig_.timestampFormat != "none")
            {
                auto time_t = std::chrono::system_clock::to_time_t(data.displayTime);
                std::tm tm = *std::localtime(&time_t);

                char timeStr[20];
                std::strftime(timeStr, sizeof(timeStr), consoleConfig_.timestampFormat.c_str(), &tm);

                oss << colorizeText("Time: ", 33) << timeStr << "\n"; // 黄色
            }

            // 根据格式类型格式化数据
            switch (data.format)
            {
            case radar::IDisplayController::DisplayFormat::CONSOLE_TEXT:
                oss << formatAsText(data);
                break;
            case radar::IDisplayController::DisplayFormat::CONSOLE_CHART:
                oss << formatAsChart(data);
                break;
            default:
                oss << formatAsText(data);
                break;
            }

            // 添加分隔线
            if (consoleConfig_.showHeaders)
            {
                oss << colorizeText(std::string(consoleConfig_.tableWidth, '-'), 37) << "\n"; // 白色
            }

            return oss.str();
        }

        std::string ConsoleDisplayController::formatAsText(const DisplayData &data)
        {
            std::ostringstream oss;

            // 格式化处理结果
            const auto &result = data.sourceResult;

            oss << colorizeText("Packet ID: ", 32) << result.sourcePacketId << "\n"; // 绿色
            oss << colorizeText("Processing Time: ", 32) << std::chrono::duration_cast<std::chrono::milliseconds>(result.processingTime.time_since_epoch()).count() << " ms\n";
            oss << colorizeText("Success: ", 32) << (result.processingSuccess ? "Yes" : "No") << "\n";
            oss << colorizeText("Processing Duration: ", 32) << std::fixed << std::setprecision(2) << result.statistics.processingDurationMs << " ms\n";

            // 显示数据大小
            oss << colorizeText("Range Profile Size: ", 32) << result.rangeProfile.size() << " elements\n";
            oss << colorizeText("Doppler Spectrum Size: ", 32) << result.dopplerSpectrum.size() << " elements\n";
            oss << colorizeText("Beamformed Data Size: ", 32) << result.beamformedData.size() << " elements\n";

            // 显示性能统计
            oss << colorizeText("CPU Usage: ", 32) << std::fixed << std::setprecision(1) << result.statistics.cpuUsagePercent << "%\n";
            oss << colorizeText("GPU Usage: ", 32) << std::fixed << std::setprecision(1) << result.statistics.gpuUsagePercent << "%\n";
            oss << colorizeText("Memory Usage: ", 32) << result.statistics.memoryUsageBytes << " bytes\n";

            return oss.str();
        }

        std::string ConsoleDisplayController::formatAsChart(const DisplayData &data)
        {
            std::ostringstream oss;

            const auto &result = data.sourceResult;

            // 创建简单的文本图表
            oss << colorizeText("=== Radar Processing Results Chart ===\n", 36); // 青色

            // 处理状态指示器
            oss << colorizeText("Status: ", 32);
            if (result.processingSuccess)
            {
                oss << colorizeText("[SUCCESS]", 32) << "\n"; // 绿色
            }
            else
            {
                oss << colorizeText("[FAILED]", 31) << "\n"; // 红色
            }

            // 性能指标条形图
            oss << colorizeText("Performance Metrics:\n", 32);
            oss << createBarChart("CPU Usage", result.statistics.cpuUsagePercent, 100.0f, 32);
            oss << createBarChart("GPU Usage", result.statistics.gpuUsagePercent, 100.0f, 32);
            oss << createBarChart("Memory Usage", static_cast<float>(result.statistics.memoryUsageBytes) / (1024.0f * 1024.0f), 100.0f, 32);

            // 数据大小信息
            oss << colorizeText("Data Sizes:\n", 32);
            oss << "  Range Profile: " << result.rangeProfile.size() << " samples\n";
            oss << "  Doppler Spectrum: " << result.dopplerSpectrum.size() << " bins\n";
            oss << "  Beamformed Data: " << result.beamformedData.size() << " elements\n";

            return oss.str();
        }

        std::string ConsoleDisplayController::colorizeText(const std::string &text, int colorCode)
        {
            if (!consoleConfig_.coloredOutput)
            {
                return text;
            }

            std::ostringstream oss;
            oss << "\033[" << colorCode << "m" << text << "\033[0m";
            return oss.str();
        }

    } // namespace modules
} // namespace radar
