# DataReceiver相关单元测试CMake配置
# 仅编译本目录下的测试文件，便于独立调试和开发

cmake_minimum_required(VERSION 3.16)
project(data_receiver_tests LANGUAGES CXX)

# 查找GoogleTest
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

# Radar项目依赖（假设已在顶层CMake中定义/安装）
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

set(TEST_SOURCES
    base_receiver_test.cpp
    # 如有其他测试文件可在此添加
    # test_base_receiver.cpp
)

add_executable(data_receiver_test ${TEST_SOURCES})

target_compile_features(data_receiver_test PRIVATE cxx_std_17)
target_compile_options(data_receiver_test PRIVATE -Wall -Wextra)

target_link_libraries(data_receiver_test
    PRIVATE
        radar_common
        radar_modules
        radar_application
        spdlog::spdlog
        GTest::gtest
        GTest::gtest_main
)

# 测试输出目录
set_target_properties(data_receiver_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/unit
)

# 注册CTest
enable_testing()
add_test(NAME DataReceiverTest COMMAND data_receiver_test)
