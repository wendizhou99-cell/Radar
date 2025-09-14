#ifndef CUDA_FFT_PROCESSOR_H
#define CUDA_FFT_PROCESSOR_H

#include "common/types.h"
#include "common/error_codes.h"
#include <cuda_runtime.h>
#include <cufft.h>

namespace radar {
namespace modules {

/**
 * @brief GPU加速的FFT处理器
 * 
 * 使用CUDA cuFFT库进行高性能的快速傅里叶变换计算
 */
class CudaFFTProcessor {
public:
    /**
     * @brief 构造函数
     * @param batch_size 批处理大小
     * @param signal_length 信号长度
     */
    CudaFFTProcessor(int batch_size, int signal_length);
    
    /**
     * @brief 析构函数
     */
    ~CudaFFTProcessor();
    
    /**
     * @brief 初始化CUDA FFT处理器
     * @return 错误代码
     */
    ErrorCode Initialize();
    
    /**
     * @brief 执行前向FFT
     * @param input_data 输入数据
     * @param output_data 输出数据
     * @return 错误代码
     */
    ErrorCode ExecuteForwardFFT(const ComplexFloat* input_data, ComplexFloat* output_data);
    
    /**
     * @brief 执行反向FFT
     * @param input_data 输入数据
     * @param output_data 输出数据
     * @return 错误代码
     */
    ErrorCode ExecuteInverseFFT(const ComplexFloat* input_data, ComplexFloat* output_data);
    
    /**
     * @brief 清理资源
     * @return 错误代码
     */
    ErrorCode Cleanup();

private:
    int batch_size_;
    int signal_length_;
    cufftHandle fft_plan_forward_;
    cufftHandle fft_plan_inverse_;
    ComplexFloat* d_input_buffer_;
    ComplexFloat* d_output_buffer_;
    bool initialized_;
    
    /**
     * @brief 分配GPU内存
     * @return 错误代码
     */
    ErrorCode AllocateGPUMemory();
    
    /**
     * @brief 释放GPU内存
     */
    void FreeGPUMemory();
};

} // namespace modules
} // namespace radar

#endif // CUDA_FFT_PROCESSOR_H