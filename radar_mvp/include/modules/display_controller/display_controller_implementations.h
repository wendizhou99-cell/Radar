/**
 * @file display_controller_implementations.h
 * @brief 显示控制器具体实现类定义
 *
 * 本文件定义了各种具体的显示控制器实现类，包括控制台显示、
 * 文件输出显示、图形界面显示等。每种实现都针对特定的显示需求优化。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see ConsoleDisplayController
 * @see FileDisplayController
 * @see GraphicalDisplayController
 */

#pragma once

#include "display_controller_base.h"
#include "display_controller_statistics.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace radar
{
    namespace modules
    {
        //==============================================================================
        // 控制台显示控制器
        //==============================================================================

        /**
         * @brief 控制台显示控制器
         * @details 将处理结果输出到控制台，支持彩色输出和格式化
         */
        class ConsoleDisplayController : public DisplayControllerBase
        {
        public:
            /**
             * @brief 构造函数
             */
            ConsoleDisplayController();

            /**
             * @brief 析构函数
             */
            ~ConsoleDisplayController() override;

        protected:
            //==============================================================================
            // DisplayControllerBase 虚函数实现
            //==============================================================================

            ErrorCode initializeDisplay() override;
            ErrorCode cleanupDisplay() override;
            ErrorCode renderData(const DisplayData &data) override;
            std::vector<radar::IDisplayController::DisplayFormat> getSpecificSupportedFormats() const override;
            ErrorCode saveDisplayToFile(const std::string &filePath,
                                        const DisplayData &data) override;

        private:
            /**
             * @brief 控制台输出配置
             */
            struct ConsoleConfig
            {
                bool coloredOutput = true;                ///< 是否启用彩色输出
                uint32_t maxLines = 100;                  ///< 最大显示行数
                std::string timestampFormat = "%H:%M:%S"; ///< 时间戳格式
                bool showHeaders = true;                  ///< 是否显示表头
                uint32_t tableWidth = 80;                 ///< 表格宽度
            };

            ConsoleConfig consoleConfig_;    ///< 控制台配置
            mutable std::mutex outputMutex_; ///< 输出互斥锁
            uint32_t displayedLines_;        ///< 已显示行数

            /**
             * @brief 格式化控制台输出
             * @param data 显示数据
             * @return 格式化后的字符串
             */
            std::string formatConsoleOutput(const DisplayData &data);

            /**
             * @brief 生成彩色输出
             * @param text 文本内容
             * @param colorCode ANSI颜色代码
             * @return 彩色文本
             */
            std::string colorizeText(const std::string &text, int colorCode);

            /**
             * @brief 创建表格分隔线
             * @param width 宽度
             * @return 分隔线字符串
             */
            std::string createSeparator(uint32_t width);

            /**
             * @brief 清空控制台（如果支持）
             */
            void clearConsole();

            /**
             * @brief 格式化为文本格式
             * @param data 显示数据
             * @return 格式化后的文本字符串
             */
            std::string formatAsText(const DisplayData &data);

            /**
             * @brief 格式化为图表格式
             * @param data 显示数据
             * @return 格式化后的图表字符串
             */
            std::string formatAsChart(const DisplayData &data);

            /**
             * @brief 创建条形图
             * @param label 标签
             * @param value 当前值
             * @param maxValue 最大值
             * @param colorCode 颜色代码
             * @return 条形图字符串
             */
            std::string createBarChart(const std::string &label, float value, float maxValue, int colorCode);
        };

        //==============================================================================
        // 文件显示控制器
        //==============================================================================

        /**
         * @brief 文件显示控制器
         * @details 将处理结果输出到文件，支持多种格式和文件轮转
         */
        class FileDisplayController : public DisplayControllerBase
        {
        public:
            /**
             * @brief 构造函数
             */
            FileDisplayController();

            /**
             * @brief 析构函数
             */
            ~FileDisplayController() override;

        protected:
            //==============================================================================
            // DisplayControllerBase 虚函数实现
            //==============================================================================

            ErrorCode initializeDisplay() override;
            ErrorCode cleanupDisplay() override;
            ErrorCode renderData(const DisplayData &data) override;
            std::vector<radar::IDisplayController::DisplayFormat> getSpecificSupportedFormats() const override;
            ErrorCode saveDisplayToFile(const std::string &filePath,
                                        const DisplayData &data) override;

        private:
            /**
             * @brief 文件输出配置
             */
            struct FileConfig
            {
                std::string outputDirectory = "./output"; ///< 输出目录
                std::string filePrefix = "radar_display"; ///< 文件前缀
                std::string fileExtension = ".txt";       ///< 文件扩展名
                uint64_t maxFileSize = 10 * 1024 * 1024;  ///< 最大文件大小（字节）
                uint32_t maxFileCount = 10;               ///< 最大文件数量
                bool enableRotation = true;               ///< 是否启用文件轮转
                bool enableCompression = false;           ///< 是否启用压缩
            };

            FileConfig fileConfig_;              ///< 文件配置
            std::ofstream outputFile_;           ///< 输出文件流
            mutable std::mutex fileMutex_;       ///< 文件访问互斥锁
            uint64_t currentFileSize_;           ///< 当前文件大小
            uint32_t currentFileIndex_;          ///< 当前文件索引
            radar::DisplayFormat currentFormat_; ///< 当前显示格式

            /**
             * @brief 生成输出文件路径
             * @param index 文件索引
             * @return 文件路径
             */
            std::string generateFilePath(uint32_t index);

            /**
             * @brief 轮转文件
             * @return 操作结果错误码
             */
            ErrorCode rotateFile();

            /**
             * @brief 压缩文件
             * @param filePath 文件路径
             * @return 操作结果错误码
             */
            ErrorCode compressFile(const std::string &filePath);

            /**
             * @brief 清理旧文件
             * @return 操作结果错误码
             */
            ErrorCode cleanupOldFiles();

            /**
             * @brief 创建输出目录
             * @return 操作结果错误码
             */
            ErrorCode createOutputDirectory();
        };

        //==============================================================================
        // 网络显示控制器
        //==============================================================================

        /**
         * @brief 网络显示控制器
         * @details 通过网络发送显示数据，支持多客户端连接
         */
        class NetworkDisplayController : public DisplayControllerBase
        {
        public:
            /**
             * @brief 构造函数
             */
            NetworkDisplayController();

            /**
             * @brief 析构函数
             */
            ~NetworkDisplayController() override;

        protected:
            //==============================================================================
            // DisplayControllerBase 虚函数实现
            //==============================================================================

            ErrorCode initializeDisplay() override;
            ErrorCode cleanupDisplay() override;
            ErrorCode renderData(const DisplayData &data) override;
            std::vector<radar::IDisplayController::DisplayFormat> getSpecificSupportedFormats() const override;
            ErrorCode saveDisplayToFile(const std::string &filePath,
                                        const DisplayData &data) override;

        private:
            /**
             * @brief 网络配置
             */
            struct NetworkConfig
            {
                std::string serverAddress = "0.0.0.0"; ///< 服务器地址
                uint16_t serverPort = 8080;            ///< 服务器端口
                uint32_t maxClients = 10;              ///< 最大客户端数量
                uint32_t sendTimeoutMs = 5000;         ///< 发送超时时间
                std::string protocol = "TCP";          ///< 协议类型
                bool enableSSL = false;                ///< 是否启用SSL
            };

            NetworkConfig networkConfig_;       ///< 网络配置
            mutable std::mutex clientsMutex_;   ///< 客户端列表互斥锁
            std::vector<int> connectedClients_; ///< 已连接客户端列表
            int serverSocket_;                  ///< 服务器套接字
            std::thread acceptThread_;          ///< 接受连接线程
            std::atomic<bool> running_;         ///< 服务器运行状态

            /**
             * @brief 启动网络服务器
             * @return 操作结果错误码
             */
            ErrorCode startNetworkServer();

            /**
             * @brief 停止网络服务器
             * @return 操作结果错误码
             */
            ErrorCode stopNetworkServer();

            /**
             * @brief 接受客户端连接循环
             */
            void acceptConnectionsLoop();

            /**
             * @brief 向所有客户端发送数据
             * @param data 要发送的数据
             * @return 操作结果错误码
             */
            ErrorCode broadcastToClients(const std::string &data);

            /**
             * @brief 移除断开的客户端
             * @param clientSocket 客户端套接字
             */
            void removeClient(int clientSocket);

            /**
             * @brief 格式化网络传输数据
             * @param data 显示数据
             * @return 格式化后的网络数据字符串
             */
            std::string formatNetworkData(const DisplayData &data);
        };

        //==============================================================================
        // 混合显示控制器
        //==============================================================================

        /**
         * @brief 混合显示控制器
         * @details 同时支持多种显示输出方式的组合控制器
         */
        class HybridDisplayController : public DisplayControllerBase
        {
        public:
            /**
             * @brief 构造函数
             */
            HybridDisplayController();

            /**
             * @brief 析构函数
             */
            ~HybridDisplayController() override;

            /**
             * @brief 添加子显示控制器
             * @param controller 子控制器
             * @param name 控制器名称
             * @return 操作结果错误码
             */
            ErrorCode addSubController(std::unique_ptr<DisplayControllerBase> controller,
                                       const std::string &name);

            /**
             * @brief 移除子显示控制器
             * @param name 控制器名称
             * @return 操作结果错误码
             */
            ErrorCode removeSubController(const std::string &name);

            /**
             * @brief 启用/禁用子控制器
             * @param name 控制器名称
             * @param enabled 是否启用
             * @return 操作结果错误码
             */
            ErrorCode setSubControllerEnabled(const std::string &name, bool enabled);

        protected:
            //==============================================================================
            // DisplayControllerBase 虚函数实现
            //==============================================================================

            ErrorCode initializeDisplay() override;
            ErrorCode cleanupDisplay() override;
            ErrorCode renderData(const DisplayData &data) override;
            std::vector<radar::IDisplayController::DisplayFormat> getSpecificSupportedFormats() const override;
            ErrorCode saveDisplayToFile(const std::string &filePath,
                                        const DisplayData &data) override;

        private:
            /**
             * @brief 子控制器信息
             */
            struct SubControllerInfo
            {
                std::unique_ptr<DisplayControllerBase> controller; ///< 控制器实例
                bool enabled;                                      ///< 是否启用
                std::string name;                                  ///< 控制器名称
            };

            mutable std::mutex subControllersMutex_;        ///< 子控制器互斥锁
            std::vector<SubControllerInfo> subControllers_; ///< 子控制器列表

            /**
             * @brief 对所有启用的子控制器执行操作
             * @param operation 操作函数
             * @return 操作结果错误码
             */
            ErrorCode executeOnSubControllers(std::function<ErrorCode(DisplayControllerBase &)> operation);
        };

    } // namespace modules
} // namespace radar
