#include <gtest/gtest.h>
#include "modules/cuda_fft_processor.h"
#include "common/types.h"
#include <vector>
#include <complex>
#include <cmath>

using namespace radar::modules;
using namespace radar;

class CudaFFTProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        batch_size_ = 2;
        signal_length_ = 1024;
        processor_ = std::make_unique<CudaFFTProcessor>(batch_size_, signal_length_);
        
        // 创建测试数据
        input_data_.resize(batch_size_ * signal_length_);
        output_data_.resize(batch_size_ * signal_length_);
        
        // 生成简单的正弦波测试信号
        for (int batch = 0; batch < batch_size_; ++batch) {
            for (int i = 0; i < signal_length_; ++i) {
                float frequency = 10.0f; // 10Hz信号
                float sample_rate = 1000.0f; // 1kHz采样率
                float time = i / sample_rate;
                
                float real_part = std::cos(2.0f * M_PI * frequency * time);
                float imag_part = std::sin(2.0f * M_PI * frequency * time);
                
                input_data_[batch * signal_length_ + i] = ComplexFloat(real_part, imag_part);
            }
        }
    }
    
    void TearDown() override {
        if (processor_) {
            processor_->Cleanup();
        }
    }
    
    int batch_size_;
    int signal_length_;
    std::unique_ptr<CudaFFTProcessor> processor_;
    std::vector<ComplexFloat> input_data_;
    std::vector<ComplexFloat> output_data_;
};

TEST_F(CudaFFTProcessorTest, InitializationTest) {
    // 测试初始化
    ErrorCode result = processor_->Initialize();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    
    // 重复初始化应该成功
    result = processor_->Initialize();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
}

TEST_F(CudaFFTProcessorTest, ForwardFFTTest) {
    // 初始化处理器
    ASSERT_EQ(processor_->Initialize(), SystemErrors::SUCCESS);
    
    // 执行前向FFT
    ErrorCode result = processor_->ExecuteForwardFFT(input_data_.data(), output_data_.data());
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    
    // 验证结果不为零
    bool has_non_zero = false;
    for (const auto& sample : output_data_) {
        if (std::abs(sample) > 1e-6f) {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero);
}

TEST_F(CudaFFTProcessorTest, InverseFFTTest) {
    // 初始化处理器
    ASSERT_EQ(processor_->Initialize(), SystemErrors::SUCCESS);
    
    // 执行反向FFT
    ErrorCode result = processor_->ExecuteInverseFFT(input_data_.data(), output_data_.data());
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    
    // 验证结果不为零
    bool has_non_zero = false;
    for (const auto& sample : output_data_) {
        if (std::abs(sample) > 1e-6f) {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero);
}

TEST_F(CudaFFTProcessorTest, RoundTripTest) {
    // 初始化处理器
    ASSERT_EQ(processor_->Initialize(), SystemErrors::SUCCESS);
    
    std::vector<ComplexFloat> fft_result(batch_size_ * signal_length_);
    std::vector<ComplexFloat> ifft_result(batch_size_ * signal_length_);
    
    // 前向FFT
    ErrorCode result = processor_->ExecuteForwardFFT(input_data_.data(), fft_result.data());
    ASSERT_EQ(result, SystemErrors::SUCCESS);
    
    // 反向FFT
    result = processor_->ExecuteInverseFFT(fft_result.data(), ifft_result.data());
    ASSERT_EQ(result, SystemErrors::SUCCESS);
    
    // 验证往返变换的精度 (应该恢复到原始信号，除了缩放因子)
    float scale_factor = static_cast<float>(signal_length_);
    for (int i = 0; i < batch_size_ * signal_length_; ++i) {
        ComplexFloat expected = input_data_[i] * scale_factor;
        ComplexFloat actual = ifft_result[i];
        
        EXPECT_NEAR(expected.real(), actual.real(), 1e-4f) 
            << "Real part mismatch at index " << i;
        EXPECT_NEAR(expected.imag(), actual.imag(), 1e-4f) 
            << "Imaginary part mismatch at index " << i;
    }
}

TEST_F(CudaFFTProcessorTest, UninitializedProcessorTest) {
    // 在未初始化的处理器上调用FFT应该失败
    ErrorCode result = processor_->ExecuteForwardFFT(input_data_.data(), output_data_.data());
    EXPECT_EQ(result, DataProcessorErrors::PROCESSOR_NOT_INITIALIZED);
    
    result = processor_->ExecuteInverseFFT(input_data_.data(), output_data_.data());
    EXPECT_EQ(result, DataProcessorErrors::PROCESSOR_NOT_INITIALIZED);
}

TEST_F(CudaFFTProcessorTest, CleanupTest) {
    // 初始化处理器
    ASSERT_EQ(processor_->Initialize(), SystemErrors::SUCCESS);
    
    // 清理资源
    ErrorCode result = processor_->Cleanup();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    
    // 重复清理应该成功
    result = processor_->Cleanup();
    EXPECT_EQ(result, SystemErrors::SUCCESS);
    
    // 清理后调用FFT应该失败
    result = processor_->ExecuteForwardFFT(input_data_.data(), output_data_.data());
    EXPECT_EQ(result, DataProcessorErrors::PROCESSOR_NOT_INITIALIZED);
}