/**
 * @file receiver_factory.cpp
 * @brief 数据接收器工厂实现
 *
 * 实现了数据接收器的工厂类DataReceiverFactory，提供统一的
 * 创建接口，支持模拟和硬件接收器的创建和管理。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "common/logger.h"
#include "modules/data_receiver/data_receiver_factory.h"
#include "modules/data_receiver/hardware_receiver.h"
#include "modules/data_receiver/simulation_receiver.h"

// 防止Windows宏定义与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif

#include <algorithm>
#include <sstream>

namespace radar {
namespace modules {
//==============================================================================
// DataReceiverFactory 实现
//==============================================================================

namespace DataReceiverFactory {
//==============================================================================
// 创建函数实现
//==============================================================================

std::unique_ptr<UDPDataReceiver> createUDPReceiver(const DataReceiverConfig &config,
                                                   std::shared_ptr<spdlog::logger> logger) {
    try {
        // 验证配置有效性
        if (!validateReceiverConfig(ReceiverType::UDP_RECEIVER, config)) {
            if (logger) {
                RADAR_ERROR("Invalid configuration for UDP receiver");
            }
            return nullptr;
        }

        auto receiver = std::make_unique<UDPDataReceiver>(logger);

        ErrorCode configResult = receiver->configure(config);
        if (configResult != SystemErrors::SUCCESS) {
            if (logger) {
                RADAR_ERROR("Failed to configure UDP receiver: error code 0x{:X}", static_cast<uint32_t>(configResult));
            }
            return nullptr;
        }

        if (logger) {
            RADAR_INFO("UDP receiver created successfully");
        }

        return receiver;
    } catch (const std::exception &e) {
        if (logger) {
            RADAR_ERROR("Exception creating UDP receiver: {}", e.what());
        }
        return nullptr;
    }
}

std::unique_ptr<FileDataReceiver> createFileReceiver(const DataReceiverConfig &config,
                                                     std::shared_ptr<spdlog::logger> logger) {
    try {
        // 验证配置有效性
        if (!validateReceiverConfig(ReceiverType::FILE_RECEIVER, config)) {
            if (logger) {
                RADAR_ERROR("Invalid configuration for file receiver");
            }
            return nullptr;
        }

        auto receiver = std::make_unique<FileDataReceiver>(logger);

        ErrorCode configResult = receiver->configure(config);
        if (configResult != SystemErrors::SUCCESS) {
            if (logger) {
                RADAR_ERROR("Failed to configure file receiver: error code 0x{:X}",
                            static_cast<uint32_t>(configResult));
            }
            return nullptr;
        }

        if (logger) {
            RADAR_INFO("File receiver created successfully");
        }

        return receiver;
    } catch (const std::exception &e) {
        if (logger) {
            RADAR_ERROR("Exception creating file receiver: {}", e.what());
        }
        return nullptr;
    }
}

std::unique_ptr<HardwareDataReceiver> createHardwareReceiver(const DataReceiverConfig &config,
                                                             std::shared_ptr<spdlog::logger> logger) {
    try {
        // 验证配置有效性
        if (!validateReceiverConfig(ReceiverType::HARDWARE_RECEIVER, config)) {
            if (logger) {
                RADAR_ERROR("Invalid configuration for hardware receiver");
            }
            return nullptr;
        }

        auto receiver = std::make_unique<HardwareDataReceiver>(logger);

        ErrorCode configResult = receiver->configure(config);
        if (configResult != SystemErrors::SUCCESS) {
            if (logger) {
                RADAR_ERROR("Failed to configure hardware receiver: error code 0x{:X}",
                            static_cast<uint32_t>(configResult));
            }
            return nullptr;
        }

        if (logger) {
            RADAR_INFO("Hardware receiver created successfully");
        }

        return receiver;
    } catch (const std::exception &e) {
        if (logger) {
            RADAR_ERROR("Exception creating hardware receiver: {}", e.what());
        }
        return nullptr;
    }
}

std::unique_ptr<SimulationDataReceiver> createSimulationReceiver(const DataReceiverConfig &config,
                                                                 std::shared_ptr<spdlog::logger> logger) {
    // 完整实现
    try {
        // 验证配置有效性
        if (!validateReceiverConfig(ReceiverType::SIMULATION_RECEIVER, config)) {
            if (logger) {
                RADAR_ERROR("Invalid configuration for simulation receiver");
            }
            return nullptr;
        }

        auto receiver = std::make_unique<SimulationDataReceiver>(logger);

        ErrorCode configResult = receiver->configure(config);
        if (configResult != SystemErrors::SUCCESS) {
            if (logger) {
                RADAR_ERROR("Failed to configure simulation receiver: error code 0x{:X}",
                            static_cast<uint32_t>(configResult));
            }
            return nullptr;
        }

        if (logger) {
            RADAR_INFO("Simulation receiver created successfully");
        }

        return receiver;
    } catch (const std::exception &e) {
        if (logger) {
            RADAR_ERROR("Exception creating simulation receiver: {}", e.what());
        }
        return nullptr;
    }
}

std::unique_ptr<DataReceiver> createReceiver(ReceiverType receiverType, const DataReceiverConfig &config,
                                             std::shared_ptr<spdlog::logger> logger) {
    switch (receiverType) {
        case ReceiverType::UDP_RECEIVER:
            return createUDPReceiver(config, logger);

        case ReceiverType::FILE_RECEIVER:
            return createFileReceiver(config, logger);

        case ReceiverType::HARDWARE_RECEIVER:
            return createHardwareReceiver(config, logger);

        case ReceiverType::SIMULATION_RECEIVER:
            return createSimulationReceiver(config, logger);

        case ReceiverType::AUTO_SELECT: {
            // 自动选择策略：优先硬件 -> UDP -> 模拟
            if (isReceiverTypeAvailable(ReceiverType::HARDWARE_RECEIVER)) {
                auto hardwareReceiver = createHardwareReceiver(config, logger);
                if (hardwareReceiver) {
                    if (logger) {
                        RADAR_INFO("Auto-selected hardware receiver");
                    }
                    return hardwareReceiver;
                }
            }

            if (isReceiverTypeAvailable(ReceiverType::UDP_RECEIVER)) {
                auto udpReceiver = createUDPReceiver(config, logger);
                if (udpReceiver) {
                    if (logger) {
                        RADAR_INFO("Auto-selected UDP receiver");
                    }
                    return udpReceiver;
                }
            }

            // 最后降级到模拟接收器
            if (logger) {
                RADAR_WARN("Hardware and UDP receivers not available, using simulation receiver");
            }
            return createSimulationReceiver(config, logger);
        }

        default:
            if (logger) {
                RADAR_ERROR("Unknown receiver type: {}", static_cast<int>(receiverType));
            }
            return nullptr;
    }
}

//==============================================================================
// 可用性检查函数
//==============================================================================

bool isReceiverTypeAvailable(ReceiverType receiverType) {
    switch (receiverType) {
        case ReceiverType::SIMULATION_RECEIVER:
            return true;  // 模拟接收器总是可用

        case ReceiverType::UDP_RECEIVER:
            // TODO: 检查网络接口可用性
            return true;  // 暂时假设UDP总是可用

        case ReceiverType::FILE_RECEIVER:
            return true;  // 文件接收器总是可用

        case ReceiverType::HARDWARE_RECEIVER:
#ifdef HARDWARE_SUPPORT_ENABLED
            // TODO: 检查硬件设备可用性
            // 检查设备文件存在性或SDK可用性
            return false;  // 暂时返回false，等待硬件支持完成
#else
            return false;
#endif

        case ReceiverType::AUTO_SELECT:
            return isReceiverTypeAvailable(ReceiverType::HARDWARE_RECEIVER) ||
                   isReceiverTypeAvailable(ReceiverType::UDP_RECEIVER) ||
                   isReceiverTypeAvailable(ReceiverType::SIMULATION_RECEIVER);

        default:
            return false;
    }
}

std::vector<ReceiverType> getAvailableReceiverTypes() {
    std::vector<ReceiverType> availableTypes;

    if (isReceiverTypeAvailable(ReceiverType::SIMULATION_RECEIVER)) {
        availableTypes.push_back(ReceiverType::SIMULATION_RECEIVER);
    }

    if (isReceiverTypeAvailable(ReceiverType::UDP_RECEIVER)) {
        availableTypes.push_back(ReceiverType::UDP_RECEIVER);
    }

    if (isReceiverTypeAvailable(ReceiverType::FILE_RECEIVER)) {
        availableTypes.push_back(ReceiverType::FILE_RECEIVER);
    }

    if (isReceiverTypeAvailable(ReceiverType::HARDWARE_RECEIVER)) {
        availableTypes.push_back(ReceiverType::HARDWARE_RECEIVER);
    }

    return availableTypes;
}

//==============================================================================
// 类型信息函数
//==============================================================================

std::string getReceiverTypeDescription(ReceiverType receiverType) {
    switch (receiverType) {
        case ReceiverType::UDP_RECEIVER:
            return "UDP network data receiver for real-time radar data transmission";
        case ReceiverType::FILE_RECEIVER:
            return "File data receiver for offline data processing and testing";
        case ReceiverType::HARDWARE_RECEIVER:
            return "Hardware data receiver for real radar systems";
        case ReceiverType::SIMULATION_RECEIVER:
            return "Simulation data receiver for testing and development";
        case ReceiverType::AUTO_SELECT:
            return "Automatic receiver selection (prefers hardware)";
        default:
            return "Unknown receiver type";
    }
}

std::string getReceiverTypeName(ReceiverType receiverType) {
    switch (receiverType) {
        case ReceiverType::UDP_RECEIVER:
            return "UDP";
        case ReceiverType::FILE_RECEIVER:
            return "FILE";
        case ReceiverType::HARDWARE_RECEIVER:
            return "HARDWARE";
        case ReceiverType::SIMULATION_RECEIVER:
            return "SIMULATION";
        case ReceiverType::AUTO_SELECT:
            return "AUTO";
        default:
            return "UNKNOWN";
    }
}

ReceiverType parseReceiverType(const std::string &typeName) {
    // 转换为大写进行比较
    std::string upperName = typeName;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    if (upperName == "UDP")
        return ReceiverType::UDP_RECEIVER;
    else if (upperName == "FILE")
        return ReceiverType::FILE_RECEIVER;
    else if (upperName == "HARDWARE")
        return ReceiverType::HARDWARE_RECEIVER;
    else if (upperName == "SIMULATION" || upperName == "SIM")
        return ReceiverType::SIMULATION_RECEIVER;
    else if (upperName == "AUTO")
        return ReceiverType::AUTO_SELECT;
    else
        return ReceiverType::AUTO_SELECT;  // 默认返回自动选择
}

//==============================================================================
// 配置验证和默认配置
//==============================================================================

bool validateReceiverConfig(ReceiverType receiverType, const DataReceiverConfig &config) {
    // 基础配置验证
    if (config.packetSizeBytes == 0 || config.packetSizeBytes > 65536) {
        return false;
    }

    if (config.maxQueueSize == 0 || config.maxQueueSize > 10000) {
        return false;
    }

    // 特定类型的配置验证
    switch (receiverType) {
        case ReceiverType::UDP_RECEIVER:
            // UDP配置验证 - 基于现有配置结构，暂时跳过特殊验证
            break;

        case ReceiverType::FILE_RECEIVER:
            // 文件配置验证 - 基于现有配置结构，暂时跳过特殊验证
            break;

        case ReceiverType::HARDWARE_RECEIVER:
            // 硬件配置验证 - 基于现有配置结构，暂时跳过特殊验证
            break;

        case ReceiverType::SIMULATION_RECEIVER:
            // 模拟配置验证
            if (config.generationIntervalMs == 0 || config.generationIntervalMs > 10000) {
                return false;
            }
            break;

        case ReceiverType::AUTO_SELECT:
            // 自动选择模式不需要特殊验证
            break;

        default:
            return false;
    }

    return true;
}

DataReceiverConfig getDefaultConfig(ReceiverType receiverType) {
    DataReceiverConfig config;

    // 通用默认配置
    config.packetSizeBytes = 4096;
    config.maxQueueSize = 1000;
    config.overflowPolicy = "drop_oldest";
    config.simulationEnabled = true;
    config.dataRateMbps = 100;
    config.generationIntervalMs = 100;

    // 特定类型的默认配置
    switch (receiverType) {
        case ReceiverType::UDP_RECEIVER:
            // UDP 特定配置将通过扩展配置结构处理
            config.dataRateMbps = 1000;  // 1Gbps UDP
            break;

        case ReceiverType::FILE_RECEIVER:
            // 文件特定配置
            config.generationIntervalMs = 10;  // 10ms间隔模拟实时播放
            break;

        case ReceiverType::HARDWARE_RECEIVER:
            // 硬件特定配置
            config.dataRateMbps = 500;  // 500Mbps 硬件数据流
            break;

        case ReceiverType::SIMULATION_RECEIVER:
            config.generationIntervalMs = 100;  // 100ms间隔
            config.simulationEnabled = true;
            break;

        case ReceiverType::AUTO_SELECT:
            // 自动选择使用模拟器的默认配置
            return getDefaultConfig(ReceiverType::SIMULATION_RECEIVER);

        default:
            break;
    }

    return config;
}

}  // namespace DataReceiverFactory

//==============================================================================
// ReceiverManager 实现
//==============================================================================

ReceiverManager::ReceiverManager() {
    RADAR_DEBUG("ReceiverManager constructed");
}

ReceiverManager::~ReceiverManager() {
    // 停止所有接收器
    stopAllReceivers();

    // 清理所有接收器
    std::lock_guard<std::mutex> lock(managerMutex_);
    receivers_.clear();

    RADAR_DEBUG("ReceiverManager destructed");
}

bool ReceiverManager::registerReceiver(const std::string &name, std::unique_ptr<DataReceiver> receiver) {
    if (!receiver || name.empty()) {
        RADAR_ERROR("Invalid parameters for receiver registration");
        return false;
    }

    std::lock_guard<std::mutex> lock(managerMutex_);

    // 检查名称是否已存在
    if (receivers_.find(name) != receivers_.end()) {
        RADAR_WARN("Receiver with name '{}' already exists", name);
        return false;
    }

    receivers_[name] = std::move(receiver);
    RADAR_INFO("Receiver '{}' registered successfully", name);

    return true;
}

bool ReceiverManager::unregisterReceiver(const std::string &name) {
    std::lock_guard<std::mutex> lock(managerMutex_);

    auto it = receivers_.find(name);
    if (it == receivers_.end()) {
        RADAR_WARN("Receiver '{}' not found for unregistration", name);
        return false;
    }

    // 停止接收器
    if (it->second) {
        it->second->stop();
    }

    receivers_.erase(it);
    RADAR_INFO("Receiver '{}' unregistered successfully", name);

    return true;
}

DataReceiver *ReceiverManager::getReceiver(const std::string &name) {
    std::lock_guard<std::mutex> lock(managerMutex_);

    auto it = receivers_.find(name);
    if (it != receivers_.end()) {
        return it->second.get();
    }

    return nullptr;
}

size_t ReceiverManager::startAllReceivers() {
    std::lock_guard<std::mutex> lock(managerMutex_);

    size_t successCount = 0;

    for (auto &[name, receiver] : receivers_) {
        if (receiver) {
            ErrorCode result = receiver->start();
            if (result == SystemErrors::SUCCESS) {
                successCount++;
                RADAR_INFO("Receiver '{}' started successfully", name);
            } else {
                RADAR_ERROR("Failed to start receiver '{}': error code 0x{:X}", name, static_cast<uint32_t>(result));
            }
        }
    }

    RADAR_INFO("Started {} out of {} receivers", successCount, receivers_.size());
    return successCount;
}

size_t ReceiverManager::stopAllReceivers() {
    std::lock_guard<std::mutex> lock(managerMutex_);

    size_t successCount = 0;

    for (auto &[name, receiver] : receivers_) {
        if (receiver) {
            ErrorCode result = receiver->stop();
            if (result == SystemErrors::SUCCESS) {
                successCount++;
                RADAR_INFO("Receiver '{}' stopped successfully", name);
            } else {
                RADAR_ERROR("Failed to stop receiver '{}': error code 0x{:X}", name, static_cast<uint32_t>(result));
            }
        }
    }

    RADAR_INFO("Stopped {} out of {} receivers", successCount, receivers_.size());
    return successCount;
}

std::map<std::string, ModuleState> ReceiverManager::getAllReceiverStates() const {
    std::lock_guard<std::mutex> lock(managerMutex_);

    std::map<std::string, ModuleState> states;

    for (const auto &[name, receiver] : receivers_) {
        if (receiver) {
            states[name] = receiver->getState();
        } else {
            states[name] = ModuleState::ERROR;
        }
    }

    return states;
}

std::string ReceiverManager::generateStatusReport() const {
    std::lock_guard<std::mutex> lock(managerMutex_);

    std::ostringstream report;
    report << "=== Receiver Manager Status Report ===\n";
    report << "Total receivers: " << receivers_.size() << "\n\n";

    if (receivers_.empty()) {
        report << "No receivers registered.\n";
        return report.str();
    }

    // 统计各状态的接收器数量
    std::map<ModuleState, int> stateCounts;
    for (const auto &[name, receiver] : receivers_) {
        if (receiver) {
            stateCounts[receiver->getState()]++;
        } else {
            stateCounts[ModuleState::ERROR]++;
        }
    }

    // 输出状态统计
    report << "Status Summary:\n";
    for (const auto &[state, count] : stateCounts) {
        report << "  " << static_cast<int>(state) << ": " << count << " receivers\n";
    }
    report << "\n";

    // 详细接收器信息
    report << "Detailed Status:\n";
    for (const auto &[name, receiver] : receivers_) {
        report << "  " << name << ": ";
        if (receiver) {
            report << "State=" << static_cast<int>(receiver->getState());

            // 添加性能信息
            auto metrics = receiver->getPerformanceMetrics();
            if (metrics) {
                report << ", Performance available";
            }
        } else {
            report << "NULL pointer";
        }
        report << "\n";
    }

    return report.str();
}

}  // namespace modules
}  // namespace radar
