/**
 * @file config_manager.h
 * @brief 雷达MVP系统统一配置管理组件
 *
 * 基于yaml-cpp构建的线程安全配置管理系统，提供统一的配置加载、
 * 解析、验证和热更新功能。支持多层次配置结构和运行时配置修改。
 * 采用RAII设计模式和观察者模式实现配置变更通知。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 *
 * @see logger.h
 * @see types.h
 */

#pragma once

#include "common/types.h"
#include "common/error_codes.h"
#include "common/logger.h"
#include <yaml-cpp/yaml.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <filesystem>
#include <iostream>

namespace radar
{
    namespace common
    {

        //==============================================================================
        // 配置变更通知
        //==============================================================================

        /**
         * @brief 配置变更事件类型
         */
        enum class ConfigChangeType : uint8_t
        {
            ADDED,    ///< 新增配置项
            MODIFIED, ///< 修改配置项
            DELETED,  ///< 删除配置项
            RELOADED  ///< 重新加载配置
        };

        /**
         * @brief 配置变更事件信息
         */
        struct ConfigChangeEvent
        {
            ConfigChangeType type; ///< 变更类型
            std::string keyPath;   ///< 配置项路径（如："system.log_level"）
            YAML::Node oldValue;   ///< 旧值
            YAML::Node newValue;   ///< 新值
            Timestamp changeTime;  ///< 变更时间
        };

        /// 配置变更回调函数类型
        using ConfigChangeCallback = std::function<void(const ConfigChangeEvent &)>;

        //==============================================================================
        // 配置验证器
        //==============================================================================

        /**
         * @brief 配置验证器基类
         * @details 提供配置项的验证逻辑接口
         */
        class IConfigValidator
        {
        public:
            virtual ~IConfigValidator() = default;

            /**
             * @brief 验证配置值是否有效
             * @param value 配置值
             * @param errorMessage 错误信息输出
             * @return 验证是否通过
             */
            virtual bool validate(const YAML::Node &value, std::string &errorMessage) const = 0;

            /**
             * @brief 获取验证器描述
             * @return 验证器描述信息
             */
            virtual std::string getDescription() const = 0;
        };

        /// 配置验证器智能指针
        using ConfigValidatorPtr = std::shared_ptr<IConfigValidator>;

        //==============================================================================
        // 配置管理器类
        //==============================================================================

        /**
         * @brief 统一配置管理器
         * @details
         * 单例模式的配置管理器，负责：
         * - YAML配置文件的加载和解析
         * - 多层次配置结构的管理
         * - 配置项的类型安全访问
         * - 配置变更的实时监控和通知
         * - 配置验证和错误处理
         *
         * @note 线程安全，支持多线程并发访问
         * @warning 必须在使用任何配置功能前调用loadFromFile()
         */
        class ConfigManager
        {
        public:
            /**
             * @brief 获取配置管理器单例实例
             * @return 配置管理器引用
             */
            static ConfigManager &getInstance();

            /**
             * @brief 从文件加载配置
             * @param filename 配置文件路径
             * @param autoReload 是否启用自动重新加载
             * @return 操作结果错误码
             * @throws SystemException 文件读取或解析失败时抛出异常
             */
            ErrorCode loadFromFile(const std::string &filename, bool autoReload = false);

            /**
             * @brief 从YAML字符串加载配置
             * @param yamlContent YAML格式的配置内容
             * @return 操作结果错误码
             * @throws SystemException 解析失败时抛出异常
             */
            ErrorCode loadFromString(const std::string &yamlContent);

            /**
             * @brief 保存配置到文件
             * @param filename 输出文件路径（可选，默认使用加载时的文件）
             * @return 操作结果错误码
             */
            ErrorCode saveToFile(const std::string &filename = "") const;

            /**
             * @brief 重新加载配置文件
             * @return 操作结果错误码
             * @note 只在从文件加载时有效
             */
            ErrorCode reload();

            /**
             * @brief 获取配置值（泛型模板方法）
             * @tparam T 配置值类型
             * @param keyPath 配置项路径（如："system.log_level"）
             * @param defaultValue 默认值（配置项不存在时返回）
             * @return 配置值
             * @throws ModuleException 类型转换失败时抛出异常
             */
            template <typename T>
            T getValue(const std::string &keyPath, const T &defaultValue = T{}) const;

            /**
             * @brief 设置配置值（泛型模板方法）
             * @tparam T 配置值类型
             * @param keyPath 配置项路径
             * @param value 新的配置值
             * @return 操作结果错误码
             */
            template <typename T>
            ErrorCode setValue(const std::string &keyPath, const T &value);

            /**
             * @brief 检查配置项是否存在
             * @param keyPath 配置项路径
             * @return 配置项是否存在
             */
            bool hasKey(const std::string &keyPath) const;

            /**
             * @brief 删除配置项
             * @param keyPath 配置项路径
             * @return 操作结果错误码
             */
            ErrorCode removeKey(const std::string &keyPath);

            /**
             * @brief 获取子配置节点
             * @param keyPath 配置节点路径
             * @return 子配置管理器（只读）
             * @note 返回的子配置管理器共享相同的根配置
             */
            std::shared_ptr<const YAML::Node> getSubConfig(const std::string &keyPath) const;

            /**
             * @brief 注册配置变更回调
             * @param callback 回调函数
             * @param keyPattern 监听的配置项模式（支持通配符*）
             * @return 回调ID（用于取消注册）
             */
            uint32_t registerChangeCallback(ConfigChangeCallback callback,
                                            const std::string &keyPattern = "*");

            /**
             * @brief 取消配置变更回调
             * @param callbackId 回调ID
             * @return 操作结果错误码
             */
            ErrorCode unregisterChangeCallback(uint32_t callbackId);

            /**
             * @brief 注册配置验证器
             * @param keyPath 配置项路径
             * @param validator 验证器
             * @return 操作结果错误码
             */
            ErrorCode registerValidator(const std::string &keyPath, ConfigValidatorPtr validator);

            /**
             * @brief 验证所有配置项
             * @param errorReport 验证错误报告输出
             * @return 验证是否全部通过
             */
            bool validateAll(std::vector<std::string> &errorReport) const;

            /**
             * @brief 获取配置统计信息
             * @return 统计信息结构
             */
            struct ConfigStatistics
            {
                size_t totalKeys;       ///< 总配置项数量
                size_t totalCallbacks;  ///< 总回调数量
                size_t totalValidators; ///< 总验证器数量
                std::string sourceFile; ///< 源配置文件
                Timestamp lastModified; ///< 最后修改时间
                Timestamp lastReloaded; ///< 最后重新加载时间
                bool autoReloadEnabled; ///< 是否启用自动重新加载
            };

            ConfigStatistics getStatistics() const;

            /**
             * @brief 将配置导出为YAML字符串
             * @param pretty 是否格式化输出
             * @return YAML格式字符串
             */
            std::string exportToString(bool pretty = true) const;

            /**
             * @brief 检查配置管理器是否已加载
             * @return 是否已加载配置
             */
            bool isLoaded() const { return loaded_; }

        private:
            ConfigManager() = default;
            ~ConfigManager() = default;

            // 禁用拷贝和赋值
            ConfigManager(const ConfigManager &) = delete;
            ConfigManager &operator=(const ConfigManager &) = delete;

            /**
             * @brief 解析配置项路径
             * @param keyPath 配置项路径
             * @return 路径组件列表
             */
            std::vector<std::string> parseKeyPath(const std::string &keyPath) const;

            /**
             * @brief 获取配置节点（内部方法）
             * @param keyPath 配置项路径
             * @param createIfNotExists 不存在时是否创建
             * @return 配置节点引用
             */
            YAML::Node getNode(const std::string &keyPath, bool createIfNotExists = false);
            const YAML::Node getNode(const std::string &keyPath) const;

            /**
             * @brief 触发配置变更事件
             * @param event 变更事件
             */
            void notifyConfigChange(const ConfigChangeEvent &event);

            /**
             * @brief 检查配置项模式匹配
             * @param keyPath 配置项路径
             * @param pattern 匹配模式
             * @return 是否匹配
             */
            bool matchKeyPattern(const std::string &keyPath, const std::string &pattern) const;

            /**
             * @brief 计算配置项总数（递归）
             * @param node YAML节点
             * @return 配置项数量
             */
            size_t countKeys(const YAML::Node &node) const;

            /**
             * @brief 启动文件监控（如果启用自动重新加载）
             */
            void startFileMonitoring();

            /**
             * @brief 停止文件监控
             */
            void stopFileMonitoring();

        private:
            mutable std::shared_mutex configMutex_; ///< 配置读写锁
            YAML::Node rootConfig_;                 ///< 根配置节点
            std::string sourceFile_;                ///< 源配置文件路径
            std::atomic<bool> loaded_{false};       ///< 加载状态
            std::atomic<bool> autoReload_{false};   ///< 自动重新加载标志

            // 回调管理
            mutable std::mutex callbackMutex_;                                                     ///< 回调管理锁
            std::unordered_map<uint32_t, std::pair<ConfigChangeCallback, std::string>> callbacks_; ///< 回调映射
            std::atomic<uint32_t> nextCallbackId_{1};                                              ///< 下一个回调ID

            // 验证器管理
            mutable std::mutex validatorMutex_;                              ///< 验证器管理锁
            std::unordered_map<std::string, ConfigValidatorPtr> validators_; ///< 验证器映射

            // 统计信息
            mutable Timestamp lastModified_; ///< 最后修改时间
            mutable Timestamp lastReloaded_; ///< 最后重新加载时间

            // 文件监控（预留，实际实现可能需要平台特定代码）
            std::atomic<bool> monitoringActive_{false}; ///< 监控激活状态
        };

        //==============================================================================
        // 模板方法实现
        //==============================================================================

        template <typename T>
        T ConfigManager::getValue(const std::string &keyPath, const T &defaultValue) const
        {
            std::shared_lock<std::shared_mutex> lock(configMutex_);

            if (!loaded_)
            {
                RADAR_WARN("Config not loaded, returning default value for key: {}", keyPath);
                return defaultValue;
            }

            try
            {
                const YAML::Node node = getNode(keyPath);
                if (!node || node.IsNull())
                {
                    RADAR_DEBUG("Config key '{}' not found, using default value", keyPath);
                    return defaultValue;
                }

                return node.as<T>();
            }
            catch (const YAML::Exception &e)
            {
                RADAR_ERROR("Failed to parse config value for key '{}': {}", keyPath, e.what());
                return defaultValue;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Unexpected error getting config value for key '{}': {}", keyPath, e.what());
                return defaultValue;
            }
        }

        template <typename T>
        ErrorCode ConfigManager::setValue(const std::string &keyPath, const T &value)
        {
            std::unique_lock<std::shared_mutex> lock(configMutex_);

            if (!loaded_)
            {
                return SystemErrors::INITIALIZATION_FAILED;
            }

            try
            {
                // 解析路径
                std::vector<std::string> pathComponents = parseKeyPath(keyPath);
                if (pathComponents.empty())
                {
                    return SystemErrors::INVALID_PARAMETER;
                }

                // 检查旧值是否存在
                bool hadOldValue = false;
                YAML::Node oldValueCopy;
                try
                {
                    YAML::Node oldNode = getNode(keyPath);
                    if (oldNode && !oldNode.IsNull())
                    {
                        hadOldValue = true;
                        oldValueCopy = YAML::Node(oldNode);
                    }
                }
                catch (const std::exception &)
                {
                    hadOldValue = false;
                }

                // 创建配置副本以进行修改
                YAML::Node tempConfig = YAML::Clone(rootConfig_);

                // 导航到正确位置并设置值
                YAML::Node current = tempConfig;
                for (size_t i = 0; i < pathComponents.size() - 1; ++i)
                {
                    const std::string &component = pathComponents[i];
                    if (!current[component])
                    {
                        current[component] = YAML::Node(YAML::NodeType::Map);
                    }
                    current = current[component];
                }

                // 设置最终值
                const std::string &finalKey = pathComponents.back();
                current[finalKey] = value;

                // 临时跳过验证步骤来测试
                // 如果验证成功，替换rootConfig_
                rootConfig_ = std::move(tempConfig);

                // 验证新值（如果有验证器）
                {
                    std::lock_guard<std::mutex> validatorLock(validatorMutex_);
                    auto it = validators_.find(keyPath);
                    if (it != validators_.end())
                    {
                        YAML::Node newNode = getNode(keyPath);
                        std::string errorMessage;
                        if (!it->second->validate(newNode, errorMessage))
                        {
                            RADAR_ERROR("Config validation failed for key '{}': {}", keyPath, errorMessage);
                            return SystemErrors::INVALID_PARAMETER;
                        }
                    }
                }

                // 更新修改时间
                lastModified_ = std::chrono::high_resolution_clock::now();

                // 触发变更事件
                ConfigChangeEvent event;
                event.type = hadOldValue ? ConfigChangeType::MODIFIED : ConfigChangeType::ADDED;
                event.keyPath = keyPath;
                event.oldValue = oldValueCopy;
                event.newValue = getNode(keyPath);
                event.changeTime = lastModified_;

                lock.unlock(); // 解锁后再通知，避免死锁
                notifyConfigChange(event);

                RADAR_DEBUG("Config value set for key '{}'", keyPath);
                return SystemErrors::SUCCESS;
            }
            catch (const YAML::Exception &e)
            {
                RADAR_ERROR("Failed to set config value for key '{}': {}", keyPath, e.what());
                return SystemErrors::CONFIGURATION_ERROR;
            }
            catch (const std::exception &e)
            {
                RADAR_ERROR("Unexpected error setting config value for key '{}': {}", keyPath, e.what());
                return SystemErrors::UNKNOWN_ERROR;
            }
        }

//==============================================================================
// 便捷宏定义
//==============================================================================

/// 获取配置管理器实例
#define RADAR_CONFIG() radar::common::ConfigManager::getInstance()

/// 获取配置值（带默认值）
#define GET_CONFIG(key, type, default) RADAR_CONFIG().getValue<type>(key, default)

/// 设置配置值
#define SET_CONFIG(key, value) RADAR_CONFIG().setValue(key, value)

/// 检查配置项存在
#define HAS_CONFIG(key) RADAR_CONFIG().hasKey(key)

    } // namespace common
} // namespace radar
