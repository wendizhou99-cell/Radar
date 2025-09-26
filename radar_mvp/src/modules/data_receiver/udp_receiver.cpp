/**
 * @file udp_receiver.cpp
 * @brief UDP数据接收器实现
 *
 * 实现UDP数据接收器，用于从网络UDP套接字接收雷达数据。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-13
 * @since 1.0
 */

#include "common/error_codes.h"
#include "common/logger.h"
#include "modules/data_receiver/data_receiver_implementations.h"

namespace radar {
namespace modules {
//==============================================================================
// UDPDataReceiver 实现
//==============================================================================

UDPDataReceiver::UDPDataReceiver(std::shared_ptr<spdlog::logger> logger) : DataReceiver(logger) {
    if (logger_) {
        logger_->info("UDPDataReceiver created");
    }
}

UDPDataReceiver::~UDPDataReceiver() {
    if (logger_) {
        logger_->info("UDPDataReceiver destroyed");
    }
}

void UDPDataReceiver::receptionLoop() {
    if (logger_) {
        logger_->info("UDP reception loop started");
    }

    // 简化的接收循环实现
    while (!shouldStop_.load()) {
        // TODO: 实现实际的UDP接收逻辑
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 模拟接收数据包
        const size_t dataSize = 1024;
        auto buffer = std::make_unique<uint8_t[]>(dataSize);

        // 模拟一些数据
        for (size_t i = 0; i < dataSize; ++i) {
            buffer[i] = static_cast<uint8_t>(i % 256);
        }

        // 解析并入队
        auto packet = parseRawDataPacket(buffer.get(), dataSize);
        if (packet) {
            enqueuePacket(packet);
        }
    }

    if (logger_) {
        logger_->info("UDP reception loop stopped");
    }
}

RawDataPacketPtr UDPDataReceiver::parseRawDataPacket(const uint8_t *data, size_t size) const {
    // 调用基类的默认实现
    return DataReceiver::parseRawDataPacket(data, size);
}

}  // namespace modules
}  // namespace radar
