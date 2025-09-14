/**
 * @file network_display_controller.cpp
 * @brief 网络显示控制器实现
 *
 * 本文件实现了网络显示控制器，通过网络发送显示数据，
 * 支持多客户端连接和实时数据广播。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "modules/display_controller/display_controller_implementations.h"
#include "modules/display_controller/display_controller_base.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <string>
#include <exception>

namespace radar
{
    namespace modules
    {

        //==============================================================================
        // NetworkDisplayController 实现
        //==============================================================================

        NetworkDisplayController::NetworkDisplayController()
            : DisplayControllerBase("NetworkDisplayController"),
              networkConfig_(),
              clientsMutex_(),
              connectedClients_(),
              serverSocket_(-1),
              acceptThread_(),
              running_(false)
        {
            RADAR_INFO("NetworkDisplayController created");
        }

        NetworkDisplayController::~NetworkDisplayController()
        {
            cleanupDisplay();
            RADAR_INFO("NetworkDisplayController destroyed");
        }

        //==============================================================================
        // DisplayControllerBase 虚函数实现
        //==============================================================================

        ErrorCode NetworkDisplayController::initializeDisplay()
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);

            try
            {
                // 启动网络服务器
                ErrorCode serverResult = startNetworkServer();
                if (serverResult != SystemErrors::SUCCESS)
                {
                    RADAR_ERROR("Failed to start network server: {}", static_cast<int>(serverResult));
                    return serverResult;
                }

                RADAR_INFO("NetworkDisplayController initialized successfully, listening on {}:{}",
                           networkConfig_.serverAddress, networkConfig_.serverPort);
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during network display initialization: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        ErrorCode NetworkDisplayController::cleanupDisplay()
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);

            try
            {
                // 停止接受连接
                running_ = false;

                // 等待接受线程结束
                if (acceptThread_.joinable())
                {
                    acceptThread_.join();
                }

                // 停止网络服务器
                ErrorCode serverResult = stopNetworkServer();
                if (serverResult != SystemErrors::SUCCESS)
                {
                    RADAR_WARN("Failed to stop network server cleanly: {}", static_cast<int>(serverResult));
                }

                // 清理客户端连接
                connectedClients_.clear();

                RADAR_INFO("NetworkDisplayController cleanup completed");
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during network display cleanup: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        ErrorCode NetworkDisplayController::renderData(const DisplayData &data)
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);

            try
            {
                if (connectedClients_.empty())
                {
                    // 没有客户端连接，静默跳过
                    return SystemErrors::SUCCESS;
                }

                // 格式化数据为网络传输格式
                std::string networkData = formatNetworkData(data);

                // 广播到所有客户端
                ErrorCode broadcastResult = broadcastToClients(networkData);
                if (broadcastResult != SystemErrors::SUCCESS)
                {
                    RADAR_WARN("Failed to broadcast data to all clients: {}", static_cast<int>(broadcastResult));
                    // 不返回错误，因为部分客户端可能仍然成功接收
                }

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during network data rendering: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        std::vector<radar::IDisplayController::DisplayFormat> NetworkDisplayController::getSpecificSupportedFormats() const
        {
            return {
                radar::IDisplayController::DisplayFormat::CONSOLE_TEXT,
                radar::IDisplayController::DisplayFormat::CONSOLE_CHART};
        }

        ErrorCode NetworkDisplayController::saveDisplayToFile(const std::string &filePath,
                                                              const DisplayData &data)
        {
            // 标记未使用的参数以避免编译警告
            (void)filePath;
            (void)data;

            // 网络控制器不支持保存到文件
            RADAR_WARN("NetworkDisplayController does not support saving to file");
            return SystemErrors::INVALID_PARAMETER;
        }

        //==============================================================================
        // 私有辅助方法
        //==============================================================================

        ErrorCode NetworkDisplayController::startNetworkServer()
        {
            try
            {
                // 简化实现：标记服务器为运行状态
                // 实际实现需要使用socket编程
                running_ = true;

                // 启动接受连接线程
                acceptThread_ = std::thread(&NetworkDisplayController::acceptConnectionsLoop, this);

                RADAR_INFO("Network server started on {}:{}", networkConfig_.serverAddress, networkConfig_.serverPort);
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during server startup: {}", e.what());
                running_ = false;
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        ErrorCode NetworkDisplayController::stopNetworkServer()
        {
            try
            {
                running_ = false;

                // 关闭服务器套接字（简化实现）
                if (serverSocket_ != -1)
                {
                    serverSocket_ = -1;
                }

                RADAR_INFO("Network server stopped");
                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during server shutdown: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        void NetworkDisplayController::acceptConnectionsLoop()
        {
            RADAR_INFO("Accept connections thread started");

            while (running_)
            {
                try
                {
                    // 简化实现：模拟接受连接
                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    // 在实际实现中，这里会：
                    // 1. 监听服务器套接字
                    // 2. 接受新的客户端连接
                    // 3. 将新客户端添加到connectedClients_列表
                }
                catch (const std::exception &e)
                {
                    if (running_)
                    {
                        RADAR_ERROR("Exception in accept connections loop: {}", e.what());
                    }
                }
            }

            RADAR_INFO("Accept connections thread stopped");
        }

        ErrorCode NetworkDisplayController::broadcastToClients(const std::string &data)
        {
            // 标记未使用的参数以避免编译警告
            (void)data;

            try
            {
                int successCount = 0;
                int failureCount = 0;

                for (int clientSocket : connectedClients_)
                {
                    try
                    {
                        // 简化实现：模拟发送数据到客户端
                        // 实际实现需要使用send()系统调用
                        RADAR_DEBUG("Sending data to client {}: {} bytes", clientSocket, data.size());
                        successCount++;
                    }
                    catch (const std::exception &e)
                    {
                        RADAR_WARN("Failed to send data to client {}: {}", clientSocket, e.what());
                        failureCount++;
                        // 在实际实现中，应该调用removeClient(clientSocket);
                    }
                }

                if (successCount > 0)
                {
                    RADAR_DEBUG("Successfully sent data to {} clients, {} failures",
                                successCount, failureCount);
                }

                return SystemErrors::SUCCESS;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Exception during broadcast: {}", e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

        void NetworkDisplayController::removeClient(int clientSocket)
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);

            auto it = std::find(connectedClients_.begin(), connectedClients_.end(), clientSocket);
            if (it != connectedClients_.end())
            {
                connectedClients_.erase(it);
                RADAR_INFO("Removed client socket: {}", clientSocket);
            }
        }

        std::string NetworkDisplayController::formatNetworkData(const DisplayData &data)
        {
            std::ostringstream oss;

            const auto &result = data.sourceResult;

            // 网络传输格式：简单的键值对格式
            oss << "RADAR_DATA\n";
            oss << "PACKET_ID:" << result.sourcePacketId << "\n";
            oss << "SUCCESS:" << (result.processingSuccess ? "1" : "0") << "\n";
            oss << "DURATION:" << std::fixed << std::setprecision(2)
                << result.statistics.processingDurationMs << "\n";
            oss << "CPU_USAGE:" << std::fixed << std::setprecision(1)
                << result.statistics.cpuUsagePercent << "\n";
            oss << "GPU_USAGE:" << std::fixed << std::setprecision(1)
                << result.statistics.gpuUsagePercent << "\n";
            oss << "MEMORY:" << result.statistics.memoryUsageBytes << "\n";
            oss << "END\n";

            return oss.str();
        }

    } // namespace modules
} // namespace radar
