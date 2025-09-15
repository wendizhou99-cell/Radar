#include "modules/cuda_fft_processor.h"
#include "common/error_codes.h"
#include <cuda_runtime.h>
#include <cufft.h>
#include <iostream>

namespace radar {
namespace modules {

CudaFFTProcessor::CudaFFTProcessor(int batch_size, int signal_length)
    : batch_size_(batch_size)
    , signal_length_(signal_length)
    , fft_plan_forward_(0)
    , fft_plan_inverse_(0)
    , d_input_buffer_(nullptr)
    , d_output_buffer_(nullptr)
    , initialized_(false) {
}

CudaFFTProcessor::~CudaFFTProcessor() {
    if (initialized_) {
        Cleanup();
    }
}

ErrorCode CudaFFTProcessor::Initialize() {
    if (initialized_) {
        return SystemErrors::SUCCESS;
    }
    
    // 分配GPU内存
    auto result = AllocateGPUMemory();
    if (result != SystemErrors::SUCCESS) {
        return result;
    }
    
    // 创建前向FFT计划
    cufftResult fft_result = cufftPlan1d(&fft_plan_forward_, signal_length_, CUFFT_C2C, batch_size_);
    if (fft_result != CUFFT_SUCCESS) {
        FreeGPUMemory();
        return DataProcessorErrors::GPU_FFT_PLAN_CREATION_FAILED;
    }
    
    // 创建反向FFT计划
    fft_result = cufftPlan1d(&fft_plan_inverse_, signal_length_, CUFFT_C2C, batch_size_);
    if (fft_result != CUFFT_SUCCESS) {
        cufftDestroy(fft_plan_forward_);
        FreeGPUMemory();
        return DataProcessorErrors::GPU_FFT_PLAN_CREATION_FAILED;
    }
    
    initialized_ = true;
    return SystemErrors::SUCCESS;
}

ErrorCode CudaFFTProcessor::ExecuteForwardFFT(const ComplexFloat* input_data, ComplexFloat* output_data) {
    if (!initialized_) {
        return DataProcessorErrors::PROCESSOR_NOT_INITIALIZED;
    }
    
    // 将数据从CPU复制到GPU
    cudaError_t cuda_result = cudaMemcpy(d_input_buffer_, input_data, 
                                        batch_size_ * signal_length_ * sizeof(ComplexFloat), 
                                        cudaMemcpyHostToDevice);
    if (cuda_result != cudaSuccess) {
        return DataProcessorErrors::GPU_MEMORY_COPY_FAILED;
    }
    
    // 执行前向FFT
    cufftResult fft_result = cufftExecC2C(fft_plan_forward_, 
                                         reinterpret_cast<cufftComplex*>(d_input_buffer_),
                                         reinterpret_cast<cufftComplex*>(d_output_buffer_),
                                         CUFFT_FORWARD);
    if (fft_result != CUFFT_SUCCESS) {
        return DataProcessorErrors::GPU_FFT_EXECUTION_FAILED;
    }
    
    // 将结果从GPU复制到CPU
    cuda_result = cudaMemcpy(output_data, d_output_buffer_,
                            batch_size_ * signal_length_ * sizeof(ComplexFloat),
                            cudaMemcpyDeviceToHost);
    if (cuda_result != cudaSuccess) {
        return DataProcessorErrors::GPU_MEMORY_COPY_FAILED;
    }
    
    return SystemErrors::SUCCESS;
}

ErrorCode CudaFFTProcessor::ExecuteInverseFFT(const ComplexFloat* input_data, ComplexFloat* output_data) {
    if (!initialized_) {
        return DataProcessorErrors::PROCESSOR_NOT_INITIALIZED;
    }
    
    // 将数据从CPU复制到GPU
    cudaError_t cuda_result = cudaMemcpy(d_input_buffer_, input_data, 
                                        batch_size_ * signal_length_ * sizeof(ComplexFloat), 
                                        cudaMemcpyHostToDevice);
    if (cuda_result != cudaSuccess) {
        return DataProcessorErrors::GPU_MEMORY_COPY_FAILED;
    }
    
    // 执行反向FFT
    cufftResult fft_result = cufftExecC2C(fft_plan_inverse_, 
                                         reinterpret_cast<cufftComplex*>(d_input_buffer_),
                                         reinterpret_cast<cufftComplex*>(d_output_buffer_),
                                         CUFFT_INVERSE);
    if (fft_result != CUFFT_SUCCESS) {
        return DataProcessorErrors::GPU_FFT_EXECUTION_FAILED;
    }
    
    // 将结果从GPU复制到CPU
    cuda_result = cudaMemcpy(output_data, d_output_buffer_,
                            batch_size_ * signal_length_ * sizeof(ComplexFloat),
                            cudaMemcpyDeviceToHost);
    if (cuda_result != cudaSuccess) {
        return DataProcessorErrors::GPU_MEMORY_COPY_FAILED;
    }
    
    return SystemErrors::SUCCESS;
}

ErrorCode CudaFFTProcessor::Cleanup() {
    if (!initialized_) {
        return SystemErrors::SUCCESS;
    }
    
    // 销毁FFT计划
    if (fft_plan_forward_ != 0) {
        cufftDestroy(fft_plan_forward_);
        fft_plan_forward_ = 0;
    }
    
    if (fft_plan_inverse_ != 0) {
        cufftDestroy(fft_plan_inverse_);
        fft_plan_inverse_ = 0;
    }
    
    // 释放GPU内存
    FreeGPUMemory();
    
    initialized_ = false;
    return SystemErrors::SUCCESS;
}

ErrorCode CudaFFTProcessor::AllocateGPUMemory() {
    size_t buffer_size = batch_size_ * signal_length_ * sizeof(ComplexFloat);
    
    // 分配输入缓冲区
    cudaError_t cuda_result = cudaMalloc(reinterpret_cast<void**>(&d_input_buffer_), buffer_size);
    if (cuda_result != cudaSuccess) {
        return DataProcessorErrors::GPU_MEMORY_ALLOCATION_FAILED;
    }
    
    // 分配输出缓冲区
    cuda_result = cudaMalloc(reinterpret_cast<void**>(&d_output_buffer_), buffer_size);
    if (cuda_result != cudaSuccess) {
        cudaFree(d_input_buffer_);
        d_input_buffer_ = nullptr;
        return DataProcessorErrors::GPU_MEMORY_ALLOCATION_FAILED;
    }
    
    return SystemErrors::SUCCESS;
}

void CudaFFTProcessor::FreeGPUMemory() {
    if (d_input_buffer_ != nullptr) {
        cudaFree(d_input_buffer_);
        d_input_buffer_ = nullptr;
    }
    
    if (d_output_buffer_ != nullptr) {
        cudaFree(d_output_buffer_);
        d_output_buffer_ = nullptr;
    }
}

} // namespace modules
} // namespace radar