#[[
 * @file CMakeLists.txt
 * @brief 雷达MVP系统主构建配置文件
 *
 * 基于GPU的相控阵雷达数据处理系统的核心构建脚本。
 * 配置项目全局设置、第三方库集成和子模块构建。
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
]]

cmake_minimum_required(VERSION 3.20)

# 项目定义
project(radar_mvp_system
    VERSION 1.0.0
    DESCRIPTION "基于GPU的相控阵雷达数据处理系统 - MVP版本"
    LANGUAGES CXX
)

# 全局编译器设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找必要的系统库
find_package(Threads REQUIRED)

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译器特定设置
if(MSVC)
    # Visual Studio 编译器设置
    add_compile_options(/W4 /WX /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC 和 Clang 编译器设置
    add_compile_options(-Wall -Wextra -pedantic -fPIC)

    # 对于第三方库，降低警告等级
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=dangling-reference")

    # 抑制特定的第三方库警告
    add_compile_options(-Wno-dangling-reference)

    # Linux特定优化
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_options(-march=native -mtune=native)
        add_compile_definitions(_GNU_SOURCE)
    endif()
endif()# Debug/Release 配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-O0 -g)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG RELEASE_BUILD)
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# CUDA 支持检测 (可选)
option(ENABLE_CUDA "启用CUDA GPU加速支持" OFF)
if(ENABLE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        message(STATUS "找到 CUDA Toolkit 版本: ${CUDAToolkit_VERSION}")
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    else()
        message(WARNING "CUDA Toolkit 未找到，将禁用GPU加速功能")
        set(ENABLE_CUDA OFF)
    endif()
endif()

# 包含目录设置
include_directories(${CMAKE_SOURCE_DIR}/include)

# 第三方库集成
add_subdirectory(third_party)

# 源代码构建
add_subdirectory(src)

# 测试构建（可选）
option(BUILD_TESTS "构建单元测试" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装配置
install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

install(DIRECTORY configs/
        DESTINATION etc/radar_mvp)

# 输出构建信息
message(STATUS "=== Radar MVP 系统构建配置 ===")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建测试: ${BUILD_TESTS}")
message(STATUS "输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "================================")
