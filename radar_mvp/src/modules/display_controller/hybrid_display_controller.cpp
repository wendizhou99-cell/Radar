/**
 * @file hybrid_display_controller.cpp
 * @brief 混合显示控制器实现
 *
 * 本文件实现了混合显示控制器，同时支持多种显示输出方式，
 * 可以组合使用控制台、文件、网络等多种显示控制器。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include <algorithm>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "modules/display_controller/display_controller_base.h"
#include "modules/display_controller/display_controller_implementations.h"

namespace radar {
namespace modules {

//==============================================================================
// HybridDisplayController 实现
//==============================================================================

HybridDisplayController::HybridDisplayController()
    : DisplayControllerBase("HybridDisplayController"), subControllersMutex_(), subControllers_() {
    RADAR_INFO("HybridDisplayController created");
}

HybridDisplayController::~HybridDisplayController() {
    cleanupDisplay();
    RADAR_INFO("HybridDisplayController destroyed");
}

//==============================================================================
// 公共接口方法
//==============================================================================

ErrorCode HybridDisplayController::addSubController(std::unique_ptr<DisplayControllerBase> controller,
                                                    const std::string &name) {
    if (!controller) {
        RADAR_ERROR("Cannot add null controller");
        return SystemErrors::INVALID_PARAMETER;
    }

    if (name.empty()) {
        RADAR_ERROR("Controller name cannot be empty");
        return SystemErrors::INVALID_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(subControllersMutex_);

    // 检查名称是否已存在
    auto it = std::find_if(subControllers_.begin(), subControllers_.end(),
                           [&name](const SubControllerInfo &info) { return info.name == name; });

    if (it != subControllers_.end()) {
        RADAR_ERROR("Controller with name '{}' already exists", name);
        return SystemErrors::INVALID_PARAMETER;
    }

    // 添加子控制器
    SubControllerInfo info;
    info.controller = std::move(controller);
    info.enabled = true;
    info.name = name;

    subControllers_.push_back(std::move(info));

    RADAR_INFO("Added sub-controller: {}", name);
    return SystemErrors::SUCCESS;
}

ErrorCode HybridDisplayController::removeSubController(const std::string &name) {
    std::lock_guard<std::mutex> lock(subControllersMutex_);

    auto it = std::find_if(subControllers_.begin(), subControllers_.end(),
                           [&name](const SubControllerInfo &info) { return info.name == name; });

    if (it == subControllers_.end()) {
        RADAR_ERROR("Controller with name '{}' not found", name);
        return SystemErrors::INVALID_PARAMETER;
    }

    // 清理控制器
    if (it->controller) {
        it->controller->cleanup();
    }

    subControllers_.erase(it);

    RADAR_INFO("Removed sub-controller: {}", name);
    return SystemErrors::SUCCESS;
}

ErrorCode HybridDisplayController::setSubControllerEnabled(const std::string &name, bool enabled) {
    std::lock_guard<std::mutex> lock(subControllersMutex_);

    auto it = std::find_if(subControllers_.begin(), subControllers_.end(),
                           [&name](const SubControllerInfo &info) { return info.name == name; });

    if (it == subControllers_.end()) {
        RADAR_ERROR("Controller with name '{}' not found", name);
        return SystemErrors::INVALID_PARAMETER;
    }

    it->enabled = enabled;

    RADAR_INFO("Set sub-controller '{}' enabled: {}", name, enabled);
    return SystemErrors::SUCCESS;
}

//==============================================================================
// DisplayControllerBase 虚函数实现
//==============================================================================

ErrorCode HybridDisplayController::initializeDisplay() {
    std::lock_guard<std::mutex> lock(subControllersMutex_);

    try {
        ErrorCode result =
            executeOnSubControllers([](DisplayControllerBase &controller) { return controller.initialize(); });

        if (result != SystemErrors::SUCCESS) {
            RADAR_ERROR("Failed to initialize some sub-controllers: {}", static_cast<int>(result));
            return result;
        }

        RADAR_INFO("HybridDisplayController initialized successfully");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during hybrid display initialization: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode HybridDisplayController::cleanupDisplay() {
    std::lock_guard<std::mutex> lock(subControllersMutex_);

    try {
        ErrorCode result =
            executeOnSubControllers([](DisplayControllerBase &controller) { return controller.cleanup(); });

        // 清理子控制器列表
        subControllers_.clear();

        if (result != SystemErrors::SUCCESS) {
            RADAR_WARN("Some sub-controllers failed to cleanup cleanly: {}", static_cast<int>(result));
        }

        RADAR_INFO("HybridDisplayController cleanup completed");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during hybrid display cleanup: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode HybridDisplayController::renderData(const DisplayData &data) {
    std::lock_guard<std::mutex> lock(subControllersMutex_);

    try {
        ErrorCode result = executeOnSubControllers([&data](DisplayControllerBase &controller) {
            return controller.displayResult(data.sourceResult, radar::IDisplayController::DisplayFormat::CONSOLE_TEXT);
        });

        if (result != SystemErrors::SUCCESS) {
            RADAR_WARN("Some sub-controllers failed to render data: {}", static_cast<int>(result));
            // 不返回错误，因为部分控制器可能仍然成功
        }

        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during hybrid data rendering: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

std::vector<radar::IDisplayController::DisplayFormat> HybridDisplayController::getSpecificSupportedFormats() const {
    // 混合控制器支持所有格式
    return {
        radar::IDisplayController::DisplayFormat::CONSOLE_TEXT, radar::IDisplayController::DisplayFormat::CONSOLE_CHART,
        radar::IDisplayController::DisplayFormat::FILE_CSV,     radar::IDisplayController::DisplayFormat::FILE_JSON,
        radar::IDisplayController::DisplayFormat::GRAPHICS_2D,  radar::IDisplayController::DisplayFormat::GRAPHICS_3D};
}

ErrorCode HybridDisplayController::saveDisplayToFile(const std::string &filePath, const DisplayData &data) {
    // 标记未使用的参数以避免编译警告
    (void)data;

    std::lock_guard<std::mutex> lock(subControllersMutex_);

    try {
        // 尝试使用第一个启用的文件控制器保存
        for (const auto &info : subControllers_) {
            if (info.enabled && info.controller) {
                // 检查控制器是否支持文件保存（简化检查）
                auto formats = info.controller->getSupportedFormats();
                bool supportsFile =
                    std::any_of(formats.begin(), formats.end(), [](radar::IDisplayController::DisplayFormat format) {
                        return format == radar::IDisplayController::DisplayFormat::FILE_CSV ||
                               format == radar::IDisplayController::DisplayFormat::FILE_JSON;
                    });

                if (supportsFile) {
                    ErrorCode result =
                        info.controller->saveToFile(filePath, radar::IDisplayController::DisplayFormat::FILE_CSV);
                    if (result == SystemErrors::SUCCESS) {
                        RADAR_INFO("Data saved to file using sub-controller: {}", info.name);
                        return SystemErrors::SUCCESS;
                    }
                }
            }
        }

        RADAR_WARN("No suitable sub-controller found for saving to file");
        return SystemErrors::RESOURCE_UNAVAILABLE;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during hybrid file saving: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

//==============================================================================
// 私有辅助方法
//==============================================================================

ErrorCode HybridDisplayController::executeOnSubControllers(
    std::function<ErrorCode(DisplayControllerBase &)> operation) {
    ErrorCode overallResult = SystemErrors::SUCCESS;

    for (auto &info : subControllers_) {
        if (info.enabled && info.controller) {
            try {
                ErrorCode result = operation(*info.controller);
                if (result != SystemErrors::SUCCESS) {
                    RADAR_WARN("Operation failed on sub-controller '{}': {}", info.name, static_cast<int>(result));
                    overallResult = result;  // 记录最后一个错误
                }
            } catch (const std::exception &e) {
                RADAR_ERROR("Exception in sub-controller '{}': {}", info.name, e.what());
                overallResult = SystemErrors::UNKNOWN_ERROR;
            }
        }
    }

    return overallResult;
}

}  // namespace modules
}  // namespace radar
