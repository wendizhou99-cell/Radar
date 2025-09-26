/**
 * @file display_controller_factory.h
 * @brief 显示控制器工厂类定义
 *
 * 本文件定义了显示控制器的工厂类，用于根据配置创建不同类型的
 * 显示控制器实例。支持策略模式和工厂模式的结合使用。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 *
 * @see DisplayControllerFactory
 * @see DisplayControllerType
 * @see DisplayControllerConfig
 */

#pragma once

#include <functional>
#include <map>
#include <memory>

#include "../../common/error_codes.h"
#include "display_controller_base.h"
#include "display_controller_implementations.h"

namespace radar {
namespace modules {
//==============================================================================
// 显示控制器类型枚举
//==============================================================================

/**
 * @brief 显示控制器类型枚举
 */
enum class DisplayControllerType {
    CONSOLE = 0,   ///< 控制台显示
    FILE = 1,      ///< 文件输出
    NETWORK = 2,   ///< 网络传输
    HYBRID = 3,    ///< 混合模式
    UNKNOWN = 999  ///< 未知类型
};

/**
 * @brief 显示控制器类型到字符串的转换
 * @param type 控制器类型
 * @return 类型字符串
 */
std::string displayControllerTypeToString(DisplayControllerType type);

/**
 * @brief 字符串到显示控制器类型的转换
 * @param typeStr 类型字符串
 * @return 控制器类型
 */
DisplayControllerType stringToDisplayControllerType(const std::string &typeStr);

//==============================================================================
// 显示控制器配置
//==============================================================================

/**
 * @brief 显示控制器配置结构
 */
struct DisplayControllerConfig {
    DisplayControllerType type = DisplayControllerType::CONSOLE;  ///< 控制器类型
    std::string name = "default";                                 ///< 控制器名称

    // 控制台配置
    struct Console {
        bool coloredOutput = true;
        uint32_t maxLines = 100;
        std::string timestampFormat = "%H:%M:%S";
        bool showHeaders = true;
        uint32_t tableWidth = 80;
    } console;

    // 文件配置
    struct File {
        std::string outputDirectory = "./output";
        std::string filePrefix = "radar_display";
        std::string fileExtension = ".txt";
        uint64_t maxFileSize = 10 * 1024 * 1024;
        uint32_t maxFileCount = 10;
        bool enableRotation = true;
        bool enableCompression = false;
    } file;

    // 网络配置
    struct Network {
        std::string serverAddress = "0.0.0.0";
        uint16_t serverPort = 8080;
        uint32_t maxClients = 10;
        uint32_t sendTimeoutMs = 5000;
        std::string protocol = "TCP";
        bool enableSSL = false;
    } network;

    // 混合模式配置
    struct Hybrid {
        std::vector<DisplayControllerType> subControllerTypes;
        std::map<std::string, bool> subControllerEnabled;
    } hybrid;

    /**
     * @brief 从YAML配置加载
     * @param yamlNode YAML节点
     * @return 操作结果错误码
     */
    ErrorCode loadFromYaml(const void *yamlNode);

    /**
     * @brief 验证配置有效性
     * @return 操作结果错误码
     */
    ErrorCode validate() const;
};

//==============================================================================
// 显示控制器工厂类
//==============================================================================

/**
 * @brief 显示控制器工厂类
 * @details 使用工厂模式创建显示控制器实例，支持配置驱动的实例化
 */
class DisplayControllerFactory {
  public:
    /**
     * @brief 创建函数类型定义
     */
    using CreateFunction = std::function<std::unique_ptr<DisplayControllerBase>(const DisplayControllerConfig &)>;

    /**
     * @brief 获取工厂单例实例
     * @return 工厂实例引用
     */
    static DisplayControllerFactory &getInstance();

    /**
     * @brief 创建显示控制器
     * @param config 配置信息
     * @return 控制器实例或错误
     */
    std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createController(
        const DisplayControllerConfig &config);

    /**
     * @brief 创建指定类型的显示控制器
     * @param type 控制器类型
     * @param name 控制器名称
     * @return 控制器实例或错误
     */
    std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createController(DisplayControllerType type,
                                                                                  const std::string &name = "default");

    /**
     * @brief 注册自定义创建函数
     * @param type 控制器类型
     * @param createFunc 创建函数
     * @return 操作结果错误码
     */
    ErrorCode registerCreateFunction(DisplayControllerType type, CreateFunction createFunc);

    /**
     * @brief 注销创建函数
     * @param type 控制器类型
     * @return 操作结果错误码
     */
    ErrorCode unregisterCreateFunction(DisplayControllerType type);

    /**
     * @brief 获取支持的控制器类型列表
     * @return 支持的类型列表
     */
    std::vector<DisplayControllerType> getSupportedTypes() const;

    /**
     * @brief 检查是否支持指定类型
     * @param type 控制器类型
     * @return 是否支持
     */
    bool isTypeSupported(DisplayControllerType type) const;

    /**
     * @brief 获取默认配置
     * @param type 控制器类型
     * @return 默认配置
     */
    DisplayControllerConfig getDefaultConfig(DisplayControllerType type) const;

  private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    DisplayControllerFactory();

    /**
     * @brief 析构函数
     */
    ~DisplayControllerFactory();

    /**
     * @brief 禁用拷贝构造函数
     */
    DisplayControllerFactory(const DisplayControllerFactory &) = delete;

    /**
     * @brief 禁用赋值操作符
     */
    DisplayControllerFactory &operator=(const DisplayControllerFactory &) = delete;

    /**
     * @brief 初始化默认创建函数
     */
    void initializeDefaultCreateFunctions();

    /**
     * @brief 创建控制台显示控制器
     * @param config 配置信息
     * @return 控制器实例
     */
    static std::unique_ptr<DisplayControllerBase> createConsoleController(const DisplayControllerConfig &config);

    /**
     * @brief 创建文件显示控制器
     * @param config 配置信息
     * @return 控制器实例
     */
    static std::unique_ptr<DisplayControllerBase> createFileController(const DisplayControllerConfig &config);

    /**
     * @brief 创建网络显示控制器
     * @param config 配置信息
     * @return 控制器实例
     */
    static std::unique_ptr<DisplayControllerBase> createNetworkController(const DisplayControllerConfig &config);

    /**
     * @brief 创建混合显示控制器
     * @param config 配置信息
     * @return 控制器实例
     */
    static std::unique_ptr<DisplayControllerBase> createHybridController(const DisplayControllerConfig &config);

  private:
    mutable std::mutex factoryMutex_;                           ///< 工厂互斥锁
    std::map<DisplayControllerType, CreateFunction> creators_;  ///< 创建函数映射
};

//==============================================================================
// 便利函数
//==============================================================================

/**
 * @brief 快速创建控制台显示控制器
 * @param coloredOutput 是否启用彩色输出
 * @param maxLines 最大显示行数
 * @return 控制器实例或错误
 */
std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createConsoleDisplayController(bool coloredOutput = true,
                                                                                            uint32_t maxLines = 100);

/**
 * @brief 快速创建文件显示控制器
 * @param outputDirectory 输出目录
 * @param filePrefix 文件前缀
 * @param maxFileSize 最大文件大小
 * @return 控制器实例或错误
 */
std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createFileDisplayController(
    const std::string &outputDirectory = "./output", const std::string &filePrefix = "radar_display",
    uint64_t maxFileSize = 10 * 1024 * 1024);

/**
 * @brief 快速创建网络显示控制器
 * @param serverAddress 服务器地址
 * @param serverPort 服务器端口
 * @param maxClients 最大客户端数量
 * @return 控制器实例或错误
 */
std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createNetworkDisplayController(
    const std::string &serverAddress = "0.0.0.0", uint16_t serverPort = 8080, uint32_t maxClients = 10);

/**
 * @brief 快速创建混合显示控制器
 * @param subTypes 子控制器类型列表
 * @return 控制器实例或错误
 */
std::pair<std::unique_ptr<DisplayControllerBase>, ErrorCode> createHybridDisplayController(
    const std::vector<DisplayControllerType> &subTypes);

}  // namespace modules
}  // namespace radar
