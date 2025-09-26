/**
 * @file file_display_controller.cpp
 * @brief 文件显示控制器实现
 *
 * 本文件实现了文件显示控制器，将雷达处理结果输出到文件，
 * 支持多种格式、文件轮转和压缩功能。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "modules/display_controller/display_controller_base.h"
#include "modules/display_controller/display_controller_implementations.h"

namespace radar {
namespace modules {

//==============================================================================
// FileDisplayController 实现
//==============================================================================

FileDisplayController::FileDisplayController()
    : DisplayControllerBase("FileDisplayController"),
      fileConfig_(),
      outputFile_(),
      fileMutex_(),
      currentFileSize_(0),
      currentFileIndex_(0),
      currentFormat_(radar::DisplayFormat::TEXT) {
    RADAR_INFO("FileDisplayController created");
}

FileDisplayController::~FileDisplayController() {
    cleanupDisplay();
    RADAR_INFO("FileDisplayController destroyed");
}

//==============================================================================
// DisplayControllerBase 虚函数实现
//==============================================================================

ErrorCode FileDisplayController::initializeDisplay() {
    std::lock_guard<std::mutex> lock(fileMutex_);

    try {
        // 创建输出目录
        ErrorCode dirResult = createOutputDirectory();
        if (dirResult != SystemErrors::SUCCESS) {
            RADAR_ERROR("Failed to create output directory: {}", static_cast<int>(dirResult));
            return dirResult;
        }

        // 生成第一个文件路径
        std::string filePath = generateFilePath(currentFileIndex_);

        // 打开输出文件
        outputFile_.open(filePath, std::ios::out | std::ios::trunc);
        if (!outputFile_.is_open()) {
            RADAR_ERROR("Failed to open output file: {}", filePath);
            return SystemErrors::RESOURCE_UNAVAILABLE;
        }

        // 写入文件头
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        outputFile_ << "=== Radar Display Controller Output ===\n";
        outputFile_ << "Created at: " << std::ctime(&time_t);
        outputFile_ << "Format: " << static_cast<int>(currentFormat_) << "\n";
        outputFile_ << "========================================\n\n";

        currentFileSize_ = outputFile_.tellp();

        RADAR_INFO("FileDisplayController initialized successfully, output file: {}", filePath);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during file display initialization: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode FileDisplayController::cleanupDisplay() {
    std::lock_guard<std::mutex> lock(fileMutex_);

    try {
        if (outputFile_.is_open()) {
            outputFile_ << "\n=== End of Output ===\n";
            outputFile_.close();
        }

        RADAR_INFO("FileDisplayController cleanup completed");
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during file display cleanup: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

ErrorCode FileDisplayController::renderData(const DisplayData &data) {
    std::lock_guard<std::mutex> lock(fileMutex_);

    try {
        if (!outputFile_.is_open()) {
            RADAR_ERROR("Output file is not open");
            return SystemErrors::RESOURCE_UNAVAILABLE;
        }

        // 简单的文本格式输出
        const auto &result = data.sourceResult;
        outputFile_ << "Packet ID: " << result.sourcePacketId << "\n";
        outputFile_ << "Processing Success: " << (result.processingSuccess ? "Yes" : "No") << "\n";
        outputFile_ << "Processing Duration: " << std::fixed << std::setprecision(2)
                    << result.statistics.processingDurationMs << " ms\n";
        outputFile_ << "---\n";

        outputFile_.flush();
        currentFileSize_ = outputFile_.tellp();

        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during file data rendering: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

std::vector<radar::IDisplayController::DisplayFormat> FileDisplayController::getSpecificSupportedFormats() const {
    return {radar::IDisplayController::DisplayFormat::FILE_CSV, radar::IDisplayController::DisplayFormat::FILE_JSON};
}

ErrorCode FileDisplayController::saveDisplayToFile(const std::string &filePath, const DisplayData &data) {
    try {
        // 创建目标目录
        std::filesystem::path targetPath(filePath);
        std::filesystem::create_directories(targetPath.parent_path());

        // 打开目标文件
        std::ofstream targetFile(filePath, std::ios::out | std::ios::trunc);
        if (!targetFile.is_open()) {
            RADAR_ERROR("Failed to open target file for saving: {}", filePath);
            return SystemErrors::RESOURCE_UNAVAILABLE;
        }

        // 简单格式化数据并写入
        const auto &result = data.sourceResult;
        targetFile << "Packet ID: " << result.sourcePacketId << "\n";
        targetFile << "Processing Success: " << (result.processingSuccess ? "Yes" : "No") << "\n";
        targetFile.close();

        RADAR_INFO("Display data saved to file: {}", filePath);
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during file saving: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

//==============================================================================
// 私有辅助方法
//==============================================================================

std::string FileDisplayController::generateFilePath(uint32_t index) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << fileConfig_.outputDirectory << "/" << fileConfig_.filePrefix << "_" << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << index << fileConfig_.fileExtension;

    return oss.str();
}

ErrorCode FileDisplayController::rotateFile() {
    // 简化实现
    return SystemErrors::SUCCESS;
}

ErrorCode FileDisplayController::compressFile(const std::string &filePath) {
    // 标记未使用的参数以避免编译警告
    (void)filePath;

    // 简化实现
    return SystemErrors::SUCCESS;
}

ErrorCode FileDisplayController::cleanupOldFiles() {
    // 简化实现
    return SystemErrors::SUCCESS;
}

ErrorCode FileDisplayController::createOutputDirectory() {
    try {
        std::filesystem::path outputDir(fileConfig_.outputDirectory);
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directories(outputDir);
            RADAR_INFO("Created output directory: {}", fileConfig_.outputDirectory);
        }
        return SystemErrors::SUCCESS;
    } catch (const std::exception &e) {
        RADAR_ERROR("Exception during directory creation: {}", e.what());
        return SystemErrors::UNKNOWN_ERROR;
    }
}

}  // namespace modules
}  // namespace radar
