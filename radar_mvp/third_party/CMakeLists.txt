#[[
 * @file third_party/CMakeLists.txt
 * @brief 第三方库统一管理配置文件
 *
 * 负责集成和配置所有外部开源库，包括日志、配置解析、测试框架等。
 * 使用 FetchContent 自动下载和构建第三方库，确保版本一致性。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
]]

include(FetchContent)

# 设置 FetchContent 策略
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

#==============================================================================
# spdlog - 高性能异步日志库 (本地版本)
#==============================================================================
message(STATUS "配置 spdlog 日志库(本地版本)...")

# 使用本地下载的 spdlog
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/spdlog)

# 创建 spdlog 别名目标
add_library(radar::spdlog ALIAS spdlog)

#==============================================================================
# yaml-cpp - YAML 配置文件解析库 (本地版本)
#==============================================================================
message(STATUS "配置 yaml-cpp 解析库(本地版本)...")

# 设置 yaml-cpp 构建选项
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)

# 使用本地下载的 yaml-cpp
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp)

# 创建 yaml-cpp 别名目标 (如果目标不存在则创建)
if(TARGET yaml-cpp)
    add_library(radar::yaml-cpp ALIAS yaml-cpp)
else()
    message(WARNING "yaml-cpp 目标未找到，创建空目标")
    add_library(radar::yaml-cpp INTERFACE)
endif()

#==============================================================================
# GoogleTest - 单元测试框架
#==============================================================================
if(BUILD_TESTS)
    message(STATUS "配置 GoogleTest 测试框架...")

    # 检查本地GoogleTest是否存在且非空
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
        message(STATUS "使用本地GoogleTest...")

        # GoogleTest 配置选项
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        # 使用本地GoogleTest
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/googletest)

    else()
        message(STATUS "本地GoogleTest不存在，使用FetchContent下载...")

        # 使用FetchContent下载GoogleTest
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
            GIT_SHALLOW    TRUE
        )

        # GoogleTest 配置选项
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(googletest)
    endif()

    # 创建别名目标（检查目标是否存在）
    if(TARGET gtest)
        add_library(radar::gtest ALIAS gtest)
        message(STATUS "创建gtest别名: radar::gtest")
    else()
        message(WARNING "gtest目标不存在，无法创建别名")
    endif()

    if(TARGET gtest_main)
        add_library(radar::gtest_main ALIAS gtest_main)
        message(STATUS "创建gtest_main别名: radar::gtest_main")
    else()
        message(WARNING "gtest_main目标不存在，无法创建别名")
    endif()
endif()

#==============================================================================
# CUDA 相关配置 (如果启用)
#==============================================================================
if(ENABLE_CUDA AND CUDAToolkit_FOUND)
    message(STATUS "检测到 CUDA 支持，配置 GPU 加速...")

    # CUDA 编译选项
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    # CUDA 架构设置（兼容更多GPU）
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES "52;61;70;75;80")
    endif()

    # CUDA 优化选项
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")
    else()
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O0 -g")
    endif()
endif()

# 输出第三方库配置信息
message(STATUS "=== 第三方库配置完成 ===")
message(STATUS "spdlog: 已配置 (异步日志)")
message(STATUS "yaml-cpp: 已配置 (配置解析)")
if(BUILD_TESTS)
    message(STATUS "GoogleTest: 已配置 (单元测试)")
endif()
if(ENABLE_CUDA AND CUDAToolkit_FOUND)
    message(STATUS "CUDA: 已配置 (GPU加速)")
endif()
message(STATUS "==========================")
