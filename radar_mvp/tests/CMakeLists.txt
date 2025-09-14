# tests/CMakeLists.txt
# 测试模块构建配置文件
#
# 配置单元测试和集成测试的构建环境，集成GoogleTest框架，
# 自动发现测试用例并注册到CTest测试系统。
#
# @author Kelin
# @version 1.0
# @date 2025-09-11
# @since 1.0

# 确保测试功能已启用
if(NOT BUILD_TESTS)
    return()
endif()

# 包含 GoogleTest 模块
include(GoogleTest)

#==============================================================================
# 测试公共设置
#==============================================================================

# 测试通用编译选项
set(TEST_COMPILE_OPTIONS "")
if(MSVC)
    list(APPEND TEST_COMPILE_OPTIONS /W3)
else()
    list(APPEND TEST_COMPILE_OPTIONS -Wall -Wextra)
endif()

# 测试通用链接库
set(TEST_LINK_LIBRARIES
    radar_common        # 添加公共组件库
    radar_modules
    radar_application
    radar::spdlog
)

# 如果yaml-cpp可用，添加到测试链接库
if(TARGET yaml-cpp)
    list(APPEND TEST_LINK_LIBRARIES yaml-cpp)
elseif(TARGET radar::yaml-cpp)
    list(APPEND TEST_LINK_LIBRARIES radar::yaml-cpp)
endif()

# 添加GoogleTest库（如果可用）
if(TARGET radar::gtest)
    list(APPEND TEST_LINK_LIBRARIES radar::gtest radar::gtest_main)
    message(STATUS "使用GoogleTest别名目标")
elseif(TARGET gtest)
    list(APPEND TEST_LINK_LIBRARIES gtest gtest_main)
    message(STATUS "使用GoogleTest原始目标")
else()
    message(WARNING "GoogleTest目标未找到，测试可能无法正常工作")
endif()

#==============================================================================
# 单元测试配置
#==============================================================================

# 收集所有单元测试源文件
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit_tests/*_test.cpp")

# 为每个测试文件创建独立的可执行文件
foreach(TEST_SOURCE ${UNIT_TEST_SOURCES})
    # 获取文件名（不含路径和扩展名）
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # 创建测试可执行文件
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # 设置测试编译选项
    target_compile_options(${TEST_NAME} PRIVATE ${TEST_COMPILE_OPTIONS})

    # 设置包含目录
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # 链接测试库
    target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LINK_LIBRARIES})

    # 设置C++标准
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)

    # 自动发现并注册测试
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )

    # 设置测试输出目录
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/unit
    )
endforeach()

#==============================================================================
# 集成测试配置
#==============================================================================

# 收集所有集成测试源文件
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration_tests/*_test.cpp")

# 为每个集成测试创建独立的可执行文件
foreach(TEST_SOURCE ${INTEGRATION_TEST_SOURCES})
    # 获取文件名（不含路径和扩展名）
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    set(INTEGRATION_TEST_NAME "integration_${TEST_NAME}")

    # 创建集成测试可执行文件
    add_executable(${INTEGRATION_TEST_NAME} ${TEST_SOURCE})

    # 设置测试编译选项
    target_compile_options(${INTEGRATION_TEST_NAME} PRIVATE ${TEST_COMPILE_OPTIONS})

    # 设置包含目录
    target_include_directories(${INTEGRATION_TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # 链接测试库
    target_link_libraries(${INTEGRATION_TEST_NAME} PRIVATE ${TEST_LINK_LIBRARIES})

    # 设置C++标准
    target_compile_features(${INTEGRATION_TEST_NAME} PRIVATE cxx_std_17)

    # 自动发现并注册测试
    gtest_discover_tests(${INTEGRATION_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )

    # 设置测试输出目录
    set_target_properties(${INTEGRATION_TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/integration
    )
endforeach()

#==============================================================================
# 测试数据和配置
#==============================================================================

# 复制测试配置文件到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/configs/
     DESTINATION ${CMAKE_BINARY_DIR}/tests/configs/)

# 创建测试数据目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data)

#==============================================================================
# 自定义测试目标
#==============================================================================

# 创建运行所有单元测试的目标
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --label-regex "unit_tests"
    DEPENDS ${UNIT_TEST_TARGETS}
    COMMENT "运行所有单元测试"
    VERBATIM
)

# 创建运行所有集成测试的目标
add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --label-regex "integration_tests"
    DEPENDS ${INTEGRATION_TEST_TARGETS}
    COMMENT "运行所有集成测试"
    VERBATIM
)

# 创建运行所有测试的目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
    COMMENT "运行所有测试"
    VERBATIM
)

# 输出测试配置信息
message(STATUS "=== 测试模块配置 ===")
message(STATUS "单元测试: ${CMAKE_BINARY_DIR}/tests/unit")
message(STATUS "集成测试: ${CMAKE_BINARY_DIR}/tests/integration")
message(STATUS "测试框架: GoogleTest")
message(STATUS "自动发现: 已启用")
message(STATUS "====================")
