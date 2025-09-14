#!/bin/bash
#[[
# @file build_linux.sh
# @brief Linux/WSL环境下的自动化构建脚本
#
# 该脚本自动检测Linux环境，配置编译器和依赖，执行完整的CMake构建流程。
# 支持Debug和Release模式，可选择启用CUDA支持。
#
# 使用方法:
#   ./build_linux.sh [Debug|Release] [--enable-cuda] [--clean]
#
# @author Kelin
# @version 1.0
# @date 2025-09-13
]]

set -e  # 出错时立即退出

# 颜色输出定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_message() {
    local color=$1
    local message=$2
    echo -e "${color}[BUILD] ${message}${NC}"
}

print_info() {
    print_message "$BLUE" "$1"
}

print_success() {
    print_message "$GREEN" "$1"
}

print_warning() {
    print_message "$YELLOW" "$1"
}

print_error() {
    print_message "$RED" "$1"
}

# 默认参数
BUILD_TYPE="Release"
ENABLE_CUDA="OFF"
CLEAN_BUILD="false"
BUILD_TESTS="ON"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|Release)
            BUILD_TYPE="$1"
            shift
            ;;
        --enable-cuda)
            ENABLE_CUDA="ON"
            shift
            ;;
        --disable-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [Debug|Release] [--enable-cuda] [--disable-tests] [--clean]"
            echo "  Debug|Release    : 构建类型 (默认: Release)"
            echo "  --enable-cuda    : 启用CUDA支持 (默认: 禁用)"
            echo "  --disable-tests  : 禁用测试构建 (默认: 启用)"
            echo "  --clean          : 清理构建目录"
            exit 0
            ;;
        *)
            print_error "未知参数: $1"
            exit 1
            ;;
    esac
done

print_info "=== Radar MVP 系统 Linux 构建脚本 ==="
print_info "构建类型: $BUILD_TYPE"
print_info "CUDA支持: $ENABLE_CUDA"
print_info "构建测试: $BUILD_TESTS"
print_info "======================================"

# 检查系统环境
print_info "检查系统环境..."

# 检查是否在WSL中
if [[ -f /proc/version ]] && grep -q Microsoft /proc/version; then
    print_info "检测到WSL环境"
    IS_WSL=true
else
    print_info "检测到原生Linux环境"
    IS_WSL=false
fi

# 检查必要的工具
REQUIRED_TOOLS=("cmake" "make" "g++")
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
        print_error "$tool 未安装或不在PATH中"
        print_info "请安装必要的开发工具:"
        print_info "Ubuntu/Debian: sudo apt update && sudo apt install build-essential cmake"
        print_info "CentOS/RHEL: sudo yum groupinstall \"Development Tools\" && sudo yum install cmake"
        exit 1
    fi
done

# 检查CMake版本
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)

if [[ $CMAKE_MAJOR -lt 3 ]] || [[ $CMAKE_MAJOR -eq 3 && $CMAKE_MINOR -lt 20 ]]; then
    print_warning "CMake版本 $CMAKE_VERSION 可能过旧，建议使用3.20+版本"
fi

print_success "系统环境检查完成"

# 如果启用CUDA，检查CUDA环境
if [[ "$ENABLE_CUDA" == "ON" ]]; then
    print_info "检查CUDA环境..."
    if command -v nvcc &> /dev/null; then
        CUDA_VERSION=$(nvcc --version | grep "release" | sed 's/.*release \([0-9\.]*\).*/\1/')
        print_success "找到CUDA Toolkit 版本: $CUDA_VERSION"
    else
        print_warning "CUDA编译器(nvcc)未找到，将禁用CUDA支持"
        ENABLE_CUDA="OFF"
    fi
fi

# 获取项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# 进入项目目录
cd "$PROJECT_ROOT"

# 清理构建目录（如果需要）
if [[ "$CLEAN_BUILD" == "true" ]]; then
    print_info "清理构建目录..."
    rm -rf build/
    print_success "构建目录已清理"
fi

# 创建构建目录
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

print_info "开始CMake配置..."

# 执行CMake配置
cmake \
    -B "$BUILD_DIR" \
    -S . \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_CUDA="$ENABLE_CUDA" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -G "Unix Makefiles"

if [[ $? -ne 0 ]]; then
    print_error "CMake配置失败"
    exit 1
fi

print_success "CMake配置完成"

print_info "开始编译..."

# 获取CPU核心数用于并行编译
if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
elif [[ -f /proc/cpuinfo ]]; then
    JOBS=$(grep -c ^processor /proc/cpuinfo)
else
    JOBS=4  # 默认值
fi

print_info "使用 $JOBS 个并行编译进程"

# 执行编译
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"

if [[ $? -ne 0 ]]; then
    print_error "编译失败"
    exit 1
fi

print_success "编译完成"

# 检查生成的可执行文件
EXECUTABLE="$BUILD_DIR/bin/radar_mvp"
if [[ -f "$EXECUTABLE" ]]; then
    print_success "可执行文件生成成功: $EXECUTABLE"

    # 显示文件信息
    FILE_SIZE=$(du -h "$EXECUTABLE" | cut -f1)
    print_info "文件大小: $FILE_SIZE"

    # 检查依赖库
    print_info "检查动态库依赖:"
    ldd "$EXECUTABLE" 2>/dev/null | head -10 || true
else
    print_error "可执行文件未找到: $EXECUTABLE"
    exit 1
fi

# 运行测试（如果启用）
if [[ "$BUILD_TESTS" == "ON" ]]; then
    print_info "运行测试..."
    cd "$BUILD_DIR"
    if ctest --output-on-failure -C "$BUILD_TYPE"; then
        print_success "所有测试通过"
    else
        print_warning "部分测试失败，但构建完成"
    fi
    cd ..
fi

print_success "=== 构建完成 ==="
print_info "可执行文件: $EXECUTABLE"
print_info "运行命令: $EXECUTABLE"

# 如果在WSL中，提供Windows路径信息
if [[ "$IS_WSL" == true ]]; then
    WINDOWS_PATH=$(wslpath -w "$EXECUTABLE" 2>/dev/null || echo "无法转换路径")
    if [[ "$WINDOWS_PATH" != "无法转换路径" ]]; then
        print_info "Windows路径: $WINDOWS_PATH"
    fi
fi

print_info "================="
