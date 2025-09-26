/**
 * @file config_manager.cpp
 * @brief 配置管理器的具体实现
 *
 * 实现了基于yaml-cpp的线程安全配置管理系统，提供完整的配置
 * 生命周期管理、类型安全访问和变更通知功能。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
 */

#include "common/config_manager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <shared_mutex>
#include <sstream>

namespace radar {
namespace common {

//==============================================================================
// 静态成员和辅助函数
//==============================================================================

ConfigManager &ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

//==============================================================================
// 配置加载和保存
//==============================================================================

ErrorCode ConfigManager::loadFromFile(const std::string &filename, bool autoReload) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);

    try {
        // 检查文件是否存在
        if (!std::filesystem::exists(filename)) {
            RADAR_ERROR("Config file not found: {}", filename);
            return SystemErrors::RESOURCE_UNAVAILABLE;
        }

        // 加载YAML文件
        rootConfig_ = YAML::LoadFile(filename);
        sourceFile_ = filename;
        autoReload_ = autoReload;
        lastReloaded_ = std::chrono::high_resolution_clock::now();

        // 记录配置加载时间
        lastModified_ = std::chrono::high_resolution_clock::now();

        loaded_ = true;

        // 启动文件监控（如果需要）
        if (autoReload) {
            startFileMonitoring();
        }

        lock.unlock();

        // 触发重新加载事件
        ConfigChangeEvent event;
        event.type = ConfigChangeType::RELOADED;
        event.keyPath = "*";
        event.newValue = rootConfig_;
        event.changeTime = lastReloaded_;
        notifyConfigChange(event);

        RADAR_INFO("Config loaded successfully from: {}", filename);
        RADAR_DEBUG("Auto-reload enabled: {}", autoReload);

        return SystemErrors::SUCCESS;
    } catch (const YAML::Exception &e) {
        RADAR_ERROR("Failed to parse YAML file '{}': {}", filename, e.what());
        return SystemErrors::CONFIGURATION_ERROR;
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to load config file '{}': {}", filename, e.what());
        return SystemErrors::RESOURCE_UNAVAILABLE;
    }
}

ErrorCode ConfigManager::loadFromString(const std::string &yamlContent) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);

    try {
        rootConfig_ = YAML::Load(yamlContent);
        sourceFile_.clear();  // 标记为非文件来源
        autoReload_ = false;
        lastReloaded_ = std::chrono::high_resolution_clock::now();
        lastModified_ = lastReloaded_;
        loaded_ = true;

        lock.unlock();

        // 触发重新加载事件
        ConfigChangeEvent event;
        event.type = ConfigChangeType::RELOADED;
        event.keyPath = "*";
        event.newValue = rootConfig_;
        event.changeTime = lastReloaded_;
        notifyConfigChange(event);

        RADAR_INFO("Config loaded successfully from string");
        return SystemErrors::SUCCESS;
    } catch (const YAML::Exception &e) {
        RADAR_ERROR("Failed to parse YAML string: {}", e.what());
        return SystemErrors::CONFIGURATION_ERROR;
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to load config from string: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode ConfigManager::saveToFile(const std::string &filename) const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);

    if (!loaded_) {
        return SystemErrors::INITIALIZATION_FAILED;
    }

    std::string outputFile = filename.empty() ? sourceFile_ : filename;
    if (outputFile.empty()) {
        RADAR_ERROR("No output file specified and no source file available");
        return SystemErrors::INVALID_PARAMETER;
    }

    try {
        // 创建输出目录（如果不存在）
        std::filesystem::path outputPath(outputFile);
        std::filesystem::path outputDir = outputPath.parent_path();
        if (!outputDir.empty() && !std::filesystem::exists(outputDir)) {
            std::filesystem::create_directories(outputDir);
        }

        // 写入文件
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            RADAR_ERROR("Failed to open file for writing: {}", outputFile);
            return SystemErrors::RESOURCE_UNAVAILABLE;
        }

        file << rootConfig_;
        file.close();

        RADAR_INFO("Config saved successfully to: {}", outputFile);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to save config to file '{}': {}", outputFile, e.what());
        return SystemErrors::RESOURCE_UNAVAILABLE;
    }
}

ErrorCode ConfigManager::reload() {
    if (sourceFile_.empty()) {
        RADAR_WARN("Cannot reload config: no source file available");
        return SystemErrors::INVALID_PARAMETER;
    }

    RADAR_INFO("Reloading config from: {}", sourceFile_);
    return loadFromFile(sourceFile_, autoReload_);
}

//==============================================================================
// 配置访问和修改
//==============================================================================

bool ConfigManager::hasKey(const std::string &keyPath) const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);

    if (!loaded_) {
        return false;
    }

    try {
        std::vector<std::string> pathComponents = parseKeyPath(keyPath);
        if (pathComponents.empty()) {
            return false;
        }

        YAML::Node current = rootConfig_;
        for (const std::string &component : pathComponents) {
            if (!current[component] || current[component].IsNull()) {
                return false;
            }
            current = current[component];
        }

        return true;
    } catch (const std::exception &) {
        return false;
    }
}

ErrorCode ConfigManager::removeKey(const std::string &keyPath) {
    std::unique_lock<std::shared_mutex> lock(configMutex_);

    if (!loaded_) {
        return SystemErrors::INITIALIZATION_FAILED;
    }

    try {
        std::vector<std::string> pathComponents = parseKeyPath(keyPath);
        if (pathComponents.empty()) {
            return SystemErrors::INVALID_PARAMETER;
        }

        // 导航到父节点
        YAML::Node current = rootConfig_;
        for (size_t i = 0; i < pathComponents.size() - 1; ++i) {
            if (!current[pathComponents[i]] || current[pathComponents[i]].IsNull()) {
                return SystemErrors::INVALID_PARAMETER;  // 路径不存在
            }
            current = current[pathComponents[i]];
        }

        // 检查要删除的键是否存在
        const std::string &finalKey = pathComponents.back();
        if (!current[finalKey] || current[finalKey].IsNull()) {
            return SystemErrors::INVALID_PARAMETER;  // 键不存在
        }

        // 获取要删除的节点（用于事件通知）
        YAML::Node oldValue = YAML::Node(current[finalKey]);  // 创建副本

        // 删除键
        current.remove(finalKey);

        // 更新修改时间
        lastModified_ = std::chrono::high_resolution_clock::now();

        // 触发删除事件
        ConfigChangeEvent event;
        event.type = ConfigChangeType::DELETED;
        event.keyPath = keyPath;
        event.oldValue = oldValue;
        event.changeTime = lastModified_;

        lock.unlock();
        notifyConfigChange(event);

        RADAR_DEBUG("Config key deleted: {}", keyPath);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to remove config key '{}': {}", keyPath, e.what());
        return SystemErrors::CONFIGURATION_ERROR;
    }
}

std::shared_ptr<const YAML::Node> ConfigManager::getSubConfig(const std::string &keyPath) const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);

    if (!loaded_) {
        return nullptr;
    }

    try {
        std::vector<std::string> pathComponents = parseKeyPath(keyPath);
        if (pathComponents.empty()) {
            return std::make_shared<const YAML::Node>(rootConfig_);
        }

        YAML::Node current = rootConfig_;
        for (const std::string &component : pathComponents) {
            if (!current[component] || current[component].IsNull()) {
                return nullptr;
            }
            current = current[component];
        }

        return std::make_shared<const YAML::Node>(YAML::Node(current));
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to get sub-config for key '{}': {}", keyPath, e.what());
        return nullptr;
    }
}

//==============================================================================
// 回调管理
//==============================================================================

uint32_t ConfigManager::registerChangeCallback(ConfigChangeCallback callback, const std::string &keyPattern) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    uint32_t callbackId = nextCallbackId_++;
    callbacks_[callbackId] = std::make_pair(std::move(callback), keyPattern);

    RADAR_DEBUG("Registered config change callback with ID: {}, pattern: {}", callbackId, keyPattern);
    return callbackId;
}

ErrorCode ConfigManager::unregisterChangeCallback(uint32_t callbackId) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    auto it = callbacks_.find(callbackId);
    if (it == callbacks_.end()) {
        return SystemErrors::INVALID_PARAMETER;
    }

    callbacks_.erase(it);
    RADAR_DEBUG("Unregistered config change callback with ID: {}", callbackId);
    return SystemErrors::SUCCESS;
}

void ConfigManager::notifyConfigChange(const ConfigChangeEvent &event) {
    std::lock_guard<std::mutex> lock(callbackMutex_);

    for (const auto &[callbackId, callbackPair] : callbacks_) {
        const auto &[callback, pattern] = callbackPair;

        if (matchKeyPattern(event.keyPath, pattern)) {
            try {
                callback(event);
            } catch (const std::exception &e) {
                RADAR_ERROR("Config change callback {} threw exception: {}", callbackId, e.what());
            }
        }
    }
}

//==============================================================================
// 验证器管理
//==============================================================================

ErrorCode ConfigManager::registerValidator(const std::string &keyPath, ConfigValidatorPtr validator) {
    std::lock_guard<std::mutex> lock(validatorMutex_);

    validators_[keyPath] = validator;
    RADAR_DEBUG("Registered validator for key: {}", keyPath);
    return SystemErrors::SUCCESS;
}

bool ConfigManager::validateAll(std::vector<std::string> &errorReport) const {
    std::shared_lock<std::shared_mutex> configLock(configMutex_);
    std::lock_guard<std::mutex> validatorLock(validatorMutex_);

    if (!loaded_) {
        errorReport.push_back("Config not loaded");
        return false;
    }

    bool allValid = true;
    errorReport.clear();

    for (const auto &[keyPath, validator] : validators_) {
        try {
            const YAML::Node node = getNode(keyPath);
            if (!node || node.IsNull()) {
                errorReport.push_back("Required config key missing: " + keyPath);
                allValid = false;
                continue;
            }

            std::string errorMessage;
            if (!validator->validate(node, errorMessage)) {
                errorReport.push_back("Validation failed for key '" + keyPath + "': " + errorMessage);
                allValid = false;
            }
        } catch (const std::exception &e) {
            errorReport.push_back("Exception validating key '" + keyPath + "': " + e.what());
            allValid = false;
        }
    }

    return allValid;
}

//==============================================================================
// 统计信息和导出
//==============================================================================

ConfigManager::ConfigStatistics ConfigManager::getStatistics() const {
    std::shared_lock<std::shared_mutex> configLock(configMutex_);
    std::lock_guard<std::mutex> callbackLock(callbackMutex_);
    std::lock_guard<std::mutex> validatorLock(validatorMutex_);

    ConfigStatistics stats;
    stats.totalKeys = loaded_ ? countKeys(rootConfig_) : 0;
    stats.totalCallbacks = callbacks_.size();
    stats.totalValidators = validators_.size();
    stats.sourceFile = sourceFile_;
    stats.lastModified = lastModified_;
    stats.lastReloaded = lastReloaded_;
    stats.autoReloadEnabled = autoReload_;

    return stats;
}

std::string ConfigManager::exportToString(bool pretty) const {
    std::shared_lock<std::shared_mutex> lock(configMutex_);

    if (!loaded_) {
        return "";
    }

    try {
        std::ostringstream oss;
        if (pretty) {
            oss << "# Radar MVP System Configuration\n";
            oss << "# Generated at: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now().time_since_epoch())
                       .count()
                << "\n\n";
        }
        oss << rootConfig_;
        return oss.str();
    } catch (const std::exception &e) {
        RADAR_ERROR("Failed to export config to string: {}", e.what());
        return "";
    }
}

//==============================================================================
// 私有辅助方法
//==============================================================================

std::vector<std::string> ConfigManager::parseKeyPath(const std::string &keyPath) const {
    std::vector<std::string> components;
    std::stringstream ss(keyPath);
    std::string component;

    while (std::getline(ss, component, '.')) {
        if (!component.empty()) {
            components.push_back(component);
        }
    }

    return components;
}

YAML::Node ConfigManager::getNode(const std::string &keyPath, bool createIfNotExists) {
    std::vector<std::string> pathComponents = parseKeyPath(keyPath);
    if (pathComponents.empty()) {
        return rootConfig_;
    }

    if (!createIfNotExists) {
        // 如果不需要创建，使用与const版本相同的递归方法
        std::function<YAML::Node(const YAML::Node &, const std::vector<std::string> &, size_t)> getNodeRecursive =
            [&](const YAML::Node &node, const std::vector<std::string> &path, size_t index) -> YAML::Node {
            if (index >= path.size()) {
                return node;
            }

            if (!node[path[index]]) {
                return YAML::Node();
            }

            return getNodeRecursive(node[path[index]], path, index + 1);
        };

        return getNodeRecursive(rootConfig_, pathComponents, 0);
    } else {
        // 如果需要创建，使用迭代方法
        YAML::Node current = rootConfig_;
        for (const std::string &component : pathComponents) {
            if (!current[component]) {
                current[component] = YAML::Node();
            }
            current = current[component];
        }
        return current;
    }
}

const YAML::Node ConfigManager::getNode(const std::string &keyPath) const {
    std::vector<std::string> pathComponents = parseKeyPath(keyPath);

    if (pathComponents.empty()) {
        return rootConfig_;
    }

    // 使用递归方式访问节点，避免修改原始数据
    std::function<YAML::Node(const YAML::Node &, const std::vector<std::string> &, size_t)> getNodeRecursive =
        [&](const YAML::Node &node, const std::vector<std::string> &path, size_t index) -> YAML::Node {
        if (index >= path.size()) {
            return node;
        }

        if (!node[path[index]]) {
            return YAML::Node();
        }

        return getNodeRecursive(node[path[index]], path, index + 1);
    };

    return getNodeRecursive(rootConfig_, pathComponents, 0);
}

bool ConfigManager::matchKeyPattern(const std::string &keyPath, const std::string &pattern) const {
    if (pattern == "*") {
        return true;  // 匹配所有
    }

    // 简单的通配符匹配实现
    try {
        std::string regexPattern = pattern;
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = "^" + regexPattern + "$";

        std::regex regex(regexPattern);
        return std::regex_match(keyPath, regex);
    } catch (const std::exception &) {
        // 如果正则表达式无效，使用精确匹配
        return keyPath == pattern;
    }
}

size_t ConfigManager::countKeys(const YAML::Node &node) const {
    if (!node || node.IsNull()) {
        return 0;
    }

    size_t count = 0;
    if (node.IsMap()) {
        for (const auto &pair : node) {
            count += 1 + countKeys(pair.second);
        }
    } else if (node.IsSequence()) {
        for (const auto &item : node) {
            count += countKeys(item);
        }
    } else {
        count = 1;  // 叶子节点
    }

    return count;
}

void ConfigManager::startFileMonitoring() {
    // 文件监控的简化实现
    // 实际应用中可能需要使用操作系统特定的文件系统监控API
    monitoringActive_ = true;
    RADAR_DEBUG("File monitoring started for: {}", sourceFile_);
}

void ConfigManager::stopFileMonitoring() {
    monitoringActive_ = false;
    RADAR_DEBUG("File monitoring stopped");
}

}  // namespace common
}  // namespace radar
