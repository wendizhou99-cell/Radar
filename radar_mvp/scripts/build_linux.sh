#!/bin/bash

# 默认参数
BUILD_TYPE="Release"
ENABLE_CUDA=false
ENABLE_QT=false
CLEAN=false
VCPKG_ROOT="${VCPKG_ROOT:-/opt/vcpkg}"

# 颜色输出函数
print_success() { echo -e "\033[32m$1\033[0m"; }
print_warning() { echo -e "\033[33m$1\033[0m"; }
print_error() { echo -e "\033[31m$1\033[0m"; }
print_info() { echo -e "\033[36m$1\033[0m"; }

# 参数解析
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --enable-cuda)
            ENABLE_CUDA=true
            shift
            ;;
        --enable-qt)
            ENABLE_QT=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --vcpkg-root)
            VCPKG_ROOT="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo "选项:"
            echo "  --build-type TYPE     构建类型 (Debug|Release) [默认: Release]"
            echo "  --enable-cuda         启用 CUDA 支持"
            echo "  --enable-qt           启用 Qt 支持"
            echo "  --clean               清理构建目录"
            echo "  --vcpkg-root PATH     vcpkg 根目录 [默认: /opt/vcpkg]"
            echo "  -h, --help            显示帮助信息"
            exit 0
            ;;
        *)
            print_error "未知参数: $1"
            exit 1
            ;;
    esac
done

print_info "=== Radar MVP vcpkg 构建脚本 ==="

# 验证 vcpkg 安装
VCPKG_EXE="$VCPKG_ROOT/vcpkg"
if [[ ! -x "$VCPKG_EXE" ]]; then
    print_error "vcpkg 未找到: $VCPKG_EXE"
    print_info "请安装 vcpkg 或设置正确的 VCPKG_ROOT 环境变量"
    exit 1
fi

print_success "找到 vcpkg: $VCPKG_EXE"

# 获取项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

print_info "项目目录: $PROJECT_ROOT"

# 清理构建目录
if [[ "$CLEAN" == true ]] && [[ -d "build" ]]; then
    print_info "清理构建目录..."
    rm -rf build/
    print_success "构建目录已清理"
fi

# 创建构建目录
mkdir -p build

print_info "开始 vcpkg 包安装..."

# 安装基础包
BASE_PACKAGES=("spdlog" "yaml-cpp" "gtest" "boost-system" "boost-filesystem" "boost-thread")

for package in "${BASE_PACKAGES[@]}"; do
    print_info "安装包: $package"
    "$VCPKG_EXE" install "$package" --triplet x64-linux
    if [[ $? -ne 0 ]]; then
        print_error "包安装失败: $package"
        exit 1
    fi
done

# 安装可选包
if [[ "$ENABLE_QT" == true ]]; then
    print_info "安装 Qt 包..."
    "$VCPKG_EXE" install qt6-base qt6-charts --triplet x64-linux
    if [[ $? -ne 0 ]]; then
        print_warning "Qt 包安装失败，将禁用 Qt 支持"
        ENABLE_QT=false
    fi
fi

print_success "所有包安装完成"

print_info "开始 CMake 配置..."

# 构建 CMake 参数
CMAKE_ARGS=(
    "-B" "build"
    "-S" "."
    "-G" "Ninja"
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    "-DBUILD_TESTS=ON"
)

if [[ "$ENABLE_CUDA" == true ]]; then
    CMAKE_ARGS+=("-DENABLE_CUDA=ON")
fi

if [[ "$ENABLE_QT" == true ]]; then
    CMAKE_ARGS+=("-DENABLE_QT=ON")
fi

# 执行 CMake 配置
cmake "${CMAKE_ARGS[@]}"

if [[ $? -ne 0 ]]; then
    print_error "CMake 配置失败"
    exit 1
fi

print_success "CMake 配置完成"

print_info "开始编译..."

# 执行编译
cmake --build build --config "$BUILD_TYPE" --parallel "$(nproc)"

if [[ $? -ne 0 ]]; then
    print_error "编译失败"
    exit 1
fi

print_success "编译完成"

print_info "运行测试..."

# 运行测试
cd build
ctest --output-on-failure

if [[ $? -ne 0 ]]; then
    print_warning "部分测试失败"
else
    print_success "所有测试通过"
fi

cd "$PROJECT_ROOT"

print_success "=== 构建完成 ==="
print_info "可执行文件位置: build/bin/radar_mvp"
