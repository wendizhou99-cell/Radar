/**
 * @file simulation_receiver.cpp
 * @brief 模拟数据接收器实现
 *
 * 实现模拟数据接收器，用于生成测试用的雷达数据。
 *
 * @author Klein
 * @version 1.0
 * @date 2025-09-12
 * @since 1.0
 */

#include "modules/data_receiver/simulation_receiver.h"

#include <cmath>
#include <random>
#include <thread>

#include "common/logger.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace radar {
namespace modules {
//==============================================================================
// 构造函数和析构函数
//==============================================================================

SimulationDataReceiver::SimulationDataReceiver(std::shared_ptr<spdlog::logger> logger) : DataReceiver(logger) {
    // 初始化完成，空目标列表
}

//==============================================================================
// 公共接口实现
//==============================================================================

void SimulationDataReceiver::addSimulationTarget(double range, double velocity, double rcs, double azimuth,
                                                 double elevation) {
    SimulationTarget target;
    target.range = range;
    target.velocity = velocity;
    target.rcs = rcs;
    target.azimuth = azimuth;
    target.elevation = elevation;
    targets_.push_back(target);

    RADAR_INFO(
        "Added simulation target: range={:.1f}m, velocity={:.1f}m/s, rcs={:.2f}m², azimuth={:.1f}°, elevation={:.1f}°",
        range, velocity, rcs, azimuth, elevation);
}

void SimulationDataReceiver::clearSimulationTargets() {
    targets_.clear();
    RADAR_INFO("Cleared all simulation targets");
}

void SimulationDataReceiver::setDeterministicMode(bool deterministic) {
    deterministicMode_ = deterministic;
    RADAR_INFO("Set deterministic mode: {}", deterministic ? "enabled" : "disabled");
}

void SimulationDataReceiver::setSimulationSeed(uint32_t seed) {
    simulationSeed_ = seed;
    RADAR_INFO("Set simulation seed: {}", seed);
}

//==============================================================================
// 受保护的接口实现
//==============================================================================

void SimulationDataReceiver::receptionLoop() {
    while (!shouldStop_.load()) {
        try {
            // 生成并入队数据包
            auto packet = generateSimulatedPacket();
            if (packet) {
                enqueuePacket(packet);
            }

            // 控制数据生成频率（100Hz）
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } catch (const std::exception &e) {
            // 出现异常时退出循环
            break;
        }
    }
}

RawDataPacketPtr SimulationDataReceiver::parseRawDataPacket([[maybe_unused]] const uint8_t *data,
                                                            [[maybe_unused]] size_t size) const {
    // 对于模拟接收器，我们不需要解析原始数据，因为我们生成的就是结构化数据
    RADAR_DEBUG("parseRawDataPacket called on SimulationDataReceiver (not used in simulation mode)");
    return nullptr;
}

//==============================================================================
// 私有方法实现
//==============================================================================

RawDataPacketPtr SimulationDataReceiver::generateSimulatedPacket() {
    static uint64_t sequenceId = 1;

    auto packet = std::make_shared<RawDataPacket>();
    packet->sequenceId = sequenceId++;
    packet->timestamp = std::chrono::high_resolution_clock::now();
    packet->channelCount = 4;        // 模拟4通道雷达
    packet->samplesPerChannel = 64;  // 减少到64个采样点以提高性能
    packet->priority = PacketPriority::NORMAL;

    // 设置元数据
    packet->metadata.samplingFrequency = 100e6;
    packet->metadata.centerFrequency = 10e9;
    packet->metadata.gain = 1.0;
    packet->metadata.pulseRepetitionInterval = 1000;  // 1ms

    // 初始化I/Q数据 - 简单的零数据
    size_t totalSamples = packet->channelCount * packet->samplesPerChannel;
    packet->iqData.resize(totalSamples);

    // 生成简单的测试数据而不是复杂的计算
    for (size_t i = 0; i < totalSamples; ++i) {
        packet->iqData[i] = ComplexFloat{
            static_cast<float>(i % 100) * 0.01f,        // 简单的I分量
            static_cast<float>((i + 50) % 100) * 0.01f  // 简单的Q分量
        };
    }

    return packet;
}

std::vector<ComplexFloat> SimulationDataReceiver::generateTargetEcho(const SimulationTarget &target) {
    size_t samples = 1024;                        // 每通道采样点数
    std::vector<ComplexFloat> echo(samples * 4);  // 4通道

    // 模拟参数
    double samplingRate = 100e6;  // 100 MHz采样率
    double carrierFreq = 10e9;    // 10 GHz载波频率
    double c = 3e8;               // 光速

    // 计算延时和多普勒频移
    double delay = 2 * target.range / c;
    double dopplerShift = 2 * target.velocity * carrierFreq / c;

    for (size_t channel = 0; channel < 4; ++channel) {
        // 计算通道相位偏移（模拟天线阵列）
        double phaseOffset = channel * M_PI / 4.0;  // 45度相位差

        for (size_t i = 0; i < samples; ++i) {
            double t = i / samplingRate;

            // 考虑延时的时间
            double effectiveTime = t - delay;
            if (effectiveTime >= 0) {
                // 计算信号幅度（考虑雷达截面积）
                float amplitude = static_cast<float>(std::sqrt(target.rcs) * 0.1);

                // 生成复数信号（包含多普勒频移和相位偏移）
                double phase = 2 * M_PI * dopplerShift * effectiveTime + phaseOffset;

                size_t idx = channel * samples + i;
                echo[idx] = ComplexFloat{static_cast<float>(amplitude * std::cos(phase)),
                                         static_cast<float>(amplitude * std::sin(phase))};
            }
        }
    }

    return echo;
}

std::vector<ComplexFloat> SimulationDataReceiver::generateNoise(size_t samples) {
    std::vector<ComplexFloat> noise(samples);

    // 使用固定种子确保可重现性
    static std::mt19937 generator(simulationSeed_);
    std::normal_distribution<float> distribution(0.0f, 0.05f);  // 5%噪声水平

    for (auto &sample : noise) {
        sample = ComplexFloat{distribution(generator), distribution(generator)};
    }

    return noise;
}

std::vector<ComplexFloat> SimulationDataReceiver::generateClutter(size_t samples) {
    std::vector<ComplexFloat> clutter(samples);

    // 生成地杂波信号（低频、高幅度）
    static std::mt19937 generator(simulationSeed_ + 1000);
    std::normal_distribution<float> distribution(0.0f, 0.1f);

    for (size_t i = 0; i < samples; ++i) {
        // 低频正弦波模拟地杂波
        float phase = 2.0f * M_PI * i / samples;
        float amplitude = distribution(generator) * 0.2f;

        clutter[i] = ComplexFloat{static_cast<float>(amplitude * std::cos(phase)),
                                  static_cast<float>(amplitude * std::sin(phase))};
    }

    return clutter;
}

}  // namespace modules
}  // namespace radar
