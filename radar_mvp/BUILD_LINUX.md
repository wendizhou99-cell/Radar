# Radar MVP 系统 - Linux/WSL 构建指南

## 概述

本文档描述如何在Linux环境（包括WSL）下构建基于GPU的相控阵雷达数据处理系统。

## 系统要求

### 最低要求
- **操作系统**: Ubuntu 20.04+, CentOS 8+, Fedora 35+, 或其他主流Linux发行版
- **编译器**: GCC 9+ 或 Clang 10+
- **构建工具**: CMake 3.20+, Make
- **内存**: 8GB RAM (推荐16GB+)
- **存储**: 10GB可用空间

### 可选要求
- **CUDA**: NVIDIA GPU + CUDA Toolkit 12.0+ (用于GPU加速)
- **WSL**: Windows 10 2004+ 或 Windows 11 (用于Windows开发环境)

## 快速开始

### 1. 环境准备

#### 自动安装依赖 (推荐)
```bash
# 设置脚本执行权限
chmod +x setup_linux_dependencies.sh

# 安装基础依赖
./setup_linux_dependencies.sh

# 安装CUDA支持 (可选)
./setup_linux_dependencies.sh --cuda --dev-tools
```

#### 手动安装依赖
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake git curl wget

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake git curl wget

# Fedora
sudo dnf install gcc gcc-c++ make cmake git curl wget
```

### 2. 构建项目

#### 使用自动构建脚本 (推荐)
```bash
# 设置脚本执行权限
chmod +x build_linux.sh

# 默认Release构建
./build_linux.sh

# Debug构建
./build_linux.sh Debug

# 启用CUDA支持
./build_linux.sh Release --enable-cuda

# 清理并重新构建
./build_linux.sh Release --clean
```

#### 手动CMake构建
```bash
# 配置构建
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --config Release -j$(nproc)

# 运行测试
cd build && ctest --output-on-failure
```

### 3. Windows环境下使用WSL

如果您在Windows环境下开发，可以使用提供的PowerShell脚本:

```powershell
# 在PowerShell中运行
.\build_wsl.ps1

# 启用CUDA支持
.\build_wsl.ps1 -EnableCuda

# Debug构建
.\build_wsl.ps1 -BuildType Debug

# 清理构建
.\build_wsl.ps1 -Clean
```

## 构建选项

### CMake配置选项

| 选项               | 默认值  | 说明                     |
| ------------------ | ------- | ------------------------ |
| `CMAKE_BUILD_TYPE` | Release | 构建类型 (Debug/Release) |
| `ENABLE_CUDA`      | OFF     | 启用CUDA GPU加速         |
| `BUILD_TESTS`      | ON      | 构建单元测试             |

### 示例配置命令

```bash
# 启用所有功能的Release构建
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CUDA=ON \
    -DBUILD_TESTS=ON

# 最小化Debug构建
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_CUDA=OFF \
    -DBUILD_TESTS=OFF
```

## 故障排除

### 常见问题

#### 1. CMake版本过旧
```bash
# 症状: CMake版本低于3.20
# 解决方案: 升级CMake
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh
chmod +x cmake-3.27.0-linux-x86_64.sh
sudo ./cmake-3.27.0-linux-x86_64.sh --skip-license --prefix=/usr/local
```

#### 2. 编译器不支持C++17
```bash
# 症状: 编译错误提示C++17特性不支持
# 解决方案: 升级GCC
sudo apt install gcc-9 g++-9  # Ubuntu
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
```

#### 3. CUDA库链接失败
```bash
# 症状: CUDA相关的链接错误
# 解决方案: 检查CUDA环境变量
export PATH=/usr/local/cuda/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

#### 4. 第三方库错误
```bash
# 症状: spdlog或yaml-cpp编译错误
# 解决方案: 清理并重新初始化子模块
git submodule deinit --all -f
git submodule update --init --recursive
```

### 日志调试

如果构建失败，可以启用详细日志:

```bash
# CMake详细输出
cmake -B build -S . --verbose

# Make详细输出
make VERBOSE=1 -C build

# 或使用构建脚本的调试模式
VERBOSE=1 ./build_linux.sh Debug
```

## 构建产物

成功构建后，将生成以下文件:

```
build/
├── bin/
│   └── radar_mvp          # 主可执行文件
├── lib/
│   ├── libradar_common.a  # 公共组件库
│   ├── libradar_modules.a # 功能模块库
│   └── libradar_application.a # 应用层库
└── tests/                 # 测试可执行文件
```

## 运行系统

```bash
# 运行主程序
./build/bin/radar_mvp

# 运行测试
cd build && ctest

# 检查依赖库
ldd ./build/bin/radar_mvp
```

## 性能优化

### 编译优化
```bash
# 启用本地CPU优化
cmake -B build -S . -DCMAKE_CXX_FLAGS="-march=native -mtune=native"

# 链接时优化(LTO)
cmake -B build -S . -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### 运行时优化
```bash
# 设置线程数
export OMP_NUM_THREADS=$(nproc)

# 设置CPU亲和性
taskset -c 0-7 ./build/bin/radar_mvp
```

## 开发环境

### 推荐的开发工具
```bash
# 安装开发工具
./setup_linux_dependencies.sh --dev-tools

# 这将安装:
# - GDB调试器
# - Valgrind内存检查
# - Clang-format代码格式化
# - Clang-tidy静态分析
```

### VS Code配置

项目包含预配置的VS Code设置，支持:
- C++智能感知
- CMake集成
- 调试配置
- 任务定义

## 技术支持

如果遇到构建问题:

1. 检查系统是否满足最低要求
2. 运行依赖安装脚本确保环境完整
3. 查看构建日志中的具体错误信息
4. 参考本文档的故障排除部分

---

更新时间: 2025-09-13
维护者: Kelin
