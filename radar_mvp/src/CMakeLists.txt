#[[
 * @file src/CMakeLists.txt
 * @brief 源代码模块构建配置文件
 *
 * 管理所有源代码模块的编译和链接，包括：
 * - 公共组件 (common)
 * - 核心功能模块 (modules)
 * - 应用层组装 (application)
 * - 主程序入口 (main)
 *
 * @author Kelin
 * @version 1.0
 * @date 2025-09-11
 * @since 1.0
]]

#==============================================================================
# 公共组件库
#==============================================================================

# 收集公共组件源文件
file(GLOB_RECURSE COMMON_SOURCES
    "common/*.cpp"
)

# 创建公共组件静态库
add_library(radar_common STATIC ${COMMON_SOURCES})

# 公共组件库编译配置
target_include_directories(radar_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接第三方库
target_link_libraries(radar_common
    PUBLIC
        radar::spdlog
        Threads::Threads
)

# 如果yaml-cpp可用，链接它
if(TARGET yaml-cpp)
    target_link_libraries(radar_common PUBLIC yaml-cpp)
elseif(TARGET radar::yaml-cpp)
    target_link_libraries(radar_common PUBLIC radar::yaml-cpp)
endif()

# Linux特定库
if(UNIX AND NOT APPLE)
    target_link_libraries(radar_common PUBLIC rt dl)
endif()

# 如果启用CUDA，添加CUDA库链接
if(ENABLE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(radar_common
        PUBLIC
            CUDA::cudart
            CUDA::cufft
    )

    # 设置CUDA相关属性
    set_target_properties(radar_common PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
endif()

# 编译器特定配置
target_compile_features(radar_common PUBLIC cxx_std_17)

# 预编译头文件（可选，提高编译速度）
if(MSVC)
    target_precompile_headers(radar_common PRIVATE
        <memory>
        <vector>
        <string>
        <chrono>
        <thread>
        <mutex>
        <condition_variable>
    )
endif()

#==============================================================================
# 核心功能模块库
#==============================================================================

# 收集所有模块源文件
file(GLOB_RECURSE MODULE_SOURCES
    "modules/*.cpp"
    "modules/*.cu"
)

# 如果CUDA被禁用，排除CUDA相关源文件
if(NOT ENABLE_CUDA)
    list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*cuda.*")
endif()

# 创建模块静态库
add_library(radar_modules STATIC ${MODULE_SOURCES})

# 模块库编译配置
target_include_directories(radar_modules
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接第三方库
target_link_libraries(radar_modules
    PUBLIC
        radar_common        # 依赖公共组件库
        radar::spdlog
)

# 如果yaml-cpp可用，链接它
if(TARGET yaml-cpp)
    target_link_libraries(radar_modules PUBLIC yaml-cpp)
elseif(TARGET radar::yaml-cpp)
    target_link_libraries(radar_modules PUBLIC radar::yaml-cpp)
endif()

# 如果启用CUDA，添加CUDA库链接
if(ENABLE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(radar_modules
        PUBLIC
            CUDA::cudart
            CUDA::cufft
    )

    # 设置CUDA相关属性
    set_target_properties(radar_modules PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
endif()

# 编译器特定配置
target_compile_features(radar_modules PUBLIC cxx_std_17)

# 预编译头文件（可选，提高编译速度）
if(MSVC)
    target_precompile_headers(radar_modules PRIVATE
        <memory>
        <vector>
        <string>
        <chrono>
        <thread>
        <mutex>
        <condition_variable>
    )
endif()

#==============================================================================
# 应用层静态库
#==============================================================================

# 收集应用层源文件
file(GLOB APPLICATION_SOURCES "application/*.cpp")

# 创建应用层静态库
add_library(radar_application STATIC ${APPLICATION_SOURCES})

# 应用层编译配置
target_include_directories(radar_application
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖
target_link_libraries(radar_application
    PUBLIC
        radar_modules       # 模块库已经包含radar_common依赖
        radar::spdlog
)

# 如果yaml-cpp可用，链接它
if(TARGET yaml-cpp)
    target_link_libraries(radar_application PUBLIC yaml-cpp)
elseif(TARGET radar::yaml-cpp)
    target_link_libraries(radar_application PUBLIC radar::yaml-cpp)
endif()

target_compile_features(radar_application PUBLIC cxx_std_17)

#==============================================================================
# 主程序可执行文件
#==============================================================================

# 创建主程序
add_executable(radar_mvp_main main.cpp)

# 主程序编译配置
target_include_directories(radar_mvp_main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接所有必要的库
target_link_libraries(radar_mvp_main
    PRIVATE
        radar_application
        radar_modules
        radar::spdlog
)

# 如果yaml-cpp可用，链接它
if(TARGET yaml-cpp)
    target_link_libraries(radar_mvp_main PRIVATE yaml-cpp)
elseif(TARGET radar::yaml-cpp)
    target_link_libraries(radar_mvp_main PRIVATE radar::yaml-cpp)
endif()

# 设置输出名称
set_target_properties(radar_mvp_main PROPERTIES
    OUTPUT_NAME "radar_mvp"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_features(radar_mvp_main PRIVATE cxx_std_17)

#==============================================================================
# 安装配置
#==============================================================================

# 安装可执行文件
install(TARGETS radar_mvp_main
    RUNTIME DESTINATION bin
)

# 安装库文件（供其他项目使用）
install(TARGETS radar_common radar_modules radar_application
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# 输出构建信息
message(STATUS "=== 源代码模块配置 ===")
message(STATUS "模块库: radar_modules")
message(STATUS "应用库: radar_application")
message(STATUS "主程序: radar_mvp_main -> radar_mvp")
message(STATUS "=====================")
