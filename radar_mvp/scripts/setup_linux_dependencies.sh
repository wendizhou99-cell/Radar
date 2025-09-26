#!/bin/bash
#[[
# @file setup_linux_dependencies.sh
# @brief Linux环境依赖包自动安装脚本
#
# 该脚本自动检测Linux发行版，安装必要的开发工具和库依赖。
# 支持Ubuntu/Debian、CentOS/RHEL/Fedora等主流发行版。
#
# 使用方法: ./setup_linux_dependencies.sh [--cuda] [--dev-tools]
#
# @author Klein
# @version 1.0
# @date 2025-09-13
]]

set -e

# 颜色输出定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[SETUP] $1${NC}"
}

print_success() {
    echo -e "${GREEN}[SETUP] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[SETUP] $1${NC}"
}

print_error() {
    echo -e "${RED}[SETUP] $1${NC}"
}

# 参数解析
INSTALL_CUDA=false
INSTALL_DEV_TOOLS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --cuda)
            INSTALL_CUDA=true
            shift
            ;;
        --dev-tools)
            INSTALL_DEV_TOOLS=true
            shift
            ;;
        -h|--help)
            echo "Linux环境依赖安装脚本"
            echo ""
            echo "用法: $0 [选项...]"
            echo ""
            echo "选项:"
            echo "  --cuda        安装CUDA开发环境"
            echo "  --dev-tools   安装额外的开发工具"
            echo "  -h, --help    显示此帮助信息"
            echo ""
            exit 0
            ;;
        *)
            print_error "未知参数: $1"
            exit 1
            ;;
    esac
done

print_info "=== Linux 环境依赖安装脚本 ==="
print_info "CUDA支持: $(if $INSTALL_CUDA; then echo '是'; else echo '否'; fi)"
print_info "开发工具: $(if $INSTALL_DEV_TOOLS; then echo '是'; else echo '否'; fi)"
print_info "================================"

# 检测Linux发行版
print_info "检测Linux发行版..."

if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION=$VERSION_ID
else
    print_error "无法检测Linux发行版"
    exit 1
fi

print_info "发行版: $PRETTY_NAME"

# 检查是否以root权限运行
if [[ $EUID -eq 0 ]]; then
    SUDO=""
    print_warning "以root权限运行"
else
    SUDO="sudo"
    print_info "将使用sudo权限安装包"
fi

# 基础依赖包列表
BASIC_PACKAGES=""
CMAKE_PACKAGES=""
DEV_PACKAGES=""

case $DISTRO in
    ubuntu|debian)
        print_info "配置Ubuntu/Debian包管理器..."

        # 更新包列表
        $SUDO apt update

        BASIC_PACKAGES="build-essential gcc g++ make libc6-dev"
        CMAKE_PACKAGES="cmake cmake-data"
        DEV_PACKAGES="git curl wget unzip pkg-config"

        # 线程和数学库
        BASIC_PACKAGES="$BASIC_PACKAGES libpthread-stubs0-dev librt-dev"

        if $INSTALL_DEV_TOOLS; then
            DEV_PACKAGES="$DEV_PACKAGES gdb valgrind clang-format clang-tidy"
        fi

        PACKAGE_MANAGER="apt install -y"
        ;;

    centos|rhel|fedora)
        print_info "配置CentOS/RHEL/Fedora包管理器..."

        if command -v dnf &> /dev/null; then
            PACKAGE_MANAGER="dnf install -y"
            $SUDO dnf update -y
        else
            PACKAGE_MANAGER="yum install -y"
            $SUDO yum update -y
        fi

        BASIC_PACKAGES="gcc gcc-c++ make glibc-devel"
        CMAKE_PACKAGES="cmake"
        DEV_PACKAGES="git curl wget unzip pkgconfig"

        if $INSTALL_DEV_TOOLS; then
            DEV_PACKAGES="$DEV_PACKAGES gdb valgrind clang"
        fi
        ;;

    arch|manjaro)
        print_info "配置Arch Linux包管理器..."

        $SUDO pacman -Syu --noconfirm

        BASIC_PACKAGES="base-devel gcc make glibc"
        CMAKE_PACKAGES="cmake"
        DEV_PACKAGES="git curl wget unzip pkgconf"

        if $INSTALL_DEV_TOOLS; then
            DEV_PACKAGES="$DEV_PACKAGES gdb valgrind clang"
        fi

        PACKAGE_MANAGER="pacman -S --noconfirm"
        ;;

    *)
        print_warning "未识别的Linux发行版: $DISTRO"
        print_warning "将尝试使用通用包名..."

        # 尝试检测包管理器
        if command -v apt &> /dev/null; then
            PACKAGE_MANAGER="apt install -y"
            BASIC_PACKAGES="build-essential gcc g++ make"
        elif command -v dnf &> /dev/null; then
            PACKAGE_MANAGER="dnf install -y"
            BASIC_PACKAGES="gcc gcc-c++ make"
        elif command -v yum &> /dev/null; then
            PACKAGE_MANAGER="yum install -y"
            BASIC_PACKAGES="gcc gcc-c++ make"
        elif command -v pacman &> /dev/null; then
            PACKAGE_MANAGER="pacman -S --noconfirm"
            BASIC_PACKAGES="base-devel gcc make"
        else
            print_error "无法识别包管理器，请手动安装依赖"
            exit 1
        fi

        CMAKE_PACKAGES="cmake"
        DEV_PACKAGES="git curl wget unzip"
        ;;
esac

# 安装基础构建工具
print_info "安装基础构建工具..."
$SUDO $PACKAGE_MANAGER $BASIC_PACKAGES

# 安装CMake
print_info "安装CMake..."
$SUDO $PACKAGE_MANAGER $CMAKE_PACKAGES

# 检查CMake版本
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_success "CMake版本: $CMAKE_VERSION"

    # 检查版本是否满足要求
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)

    if [[ $CMAKE_MAJOR -lt 3 ]] || [[ $CMAKE_MAJOR -eq 3 && $CMAKE_MINOR -lt 20 ]]; then
        print_warning "CMake版本可能过旧，建议升级到3.20+版本"

        # 提供升级建议
        case $DISTRO in
            ubuntu|debian)
                print_info "Ubuntu升级CMake建议:"
                print_info "  wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh"
                print_info "  chmod +x cmake-3.27.0-linux-x86_64.sh"
                print_info "  sudo ./cmake-3.27.0-linux-x86_64.sh --skip-license --prefix=/usr/local"
                ;;
        esac
    fi
else
    print_error "CMake安装失败"
    exit 1
fi

# 安装开发工具
if $INSTALL_DEV_TOOLS; then
    print_info "安装开发工具..."
    $SUDO $PACKAGE_MANAGER $DEV_PACKAGES
fi

# 安装CUDA（如果需要）
if $INSTALL_CUDA; then
    print_info "安装CUDA开发环境..."

    case $DISTRO in
        ubuntu)
            # Ubuntu CUDA安装
            print_info "配置NVIDIA CUDA仓库..."

            # 下载CUDA仓库包
            CUDA_REPO_PKG="cuda-repo-ubuntu${VERSION_ID//.}-12-2_12.2.0-1_amd64.deb"
            CUDA_REPO_URL="https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${VERSION_ID//./}/x86_64/$CUDA_REPO_PKG"

            wget -q "$CUDA_REPO_URL" -O /tmp/cuda-repo.deb
            $SUDO dpkg -i /tmp/cuda-repo.deb
            $SUDO apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${VERSION_ID//./}/x86_64/3bf863cc.pub
            $SUDO apt update

            # 安装CUDA toolkit
            $SUDO apt install -y cuda-toolkit-12-2
            ;;

        centos|rhel|fedora)
            print_warning "CentOS/RHEL/Fedora CUDA安装需要手动配置"
            print_info "请参考: https://developer.nvidia.com/cuda-downloads"
            ;;

        *)
            print_warning "该发行版需要手动安装CUDA"
            print_info "请参考NVIDIA官方文档安装CUDA Toolkit"
            ;;
    esac
fi

# 验证安装
print_info "验证安装..."

# 检查编译器
for compiler in gcc g++ make cmake; do
    if command -v $compiler &> /dev/null; then
        VERSION_OUTPUT=$($compiler --version | head -n1)
        print_success "$compiler: $VERSION_OUTPUT"
    else
        print_error "$compiler 未正确安装"
    fi
done

# 检查CUDA（如果安装）
if $INSTALL_CUDA && command -v nvcc &> /dev/null; then
    NVCC_VERSION=$(nvcc --version | grep "release" | sed 's/.*release \([0-9\.]*\).*/\1/')
    print_success "CUDA: $NVCC_VERSION"
elif $INSTALL_CUDA; then
    print_warning "CUDA安装可能不完整，请检查环境变量"
    print_info "添加到~/.bashrc:"
    print_info "  export PATH=/usr/local/cuda/bin:\$PATH"
    print_info "  export LD_LIBRARY_PATH=/usr/local/cuda/lib64:\$LD_LIBRARY_PATH"
fi

print_success "=== 依赖安装完成 ==="
print_info "现在可以运行构建脚本:"
print_info "  ./build_linux.sh"

if $INSTALL_CUDA; then
    print_info "启用CUDA构建:"
    print_info "  ./build_linux.sh --enable-cuda"
fi
