#[[
# @file build_windows.ps1
# @brief Windows环境下使用vcpkg构建系统的PowerShell脚本
#
# 该脚本在Windows环境中使用vcpkg包管理器构建雷达MVP系统，
# 支持manifest模式和经典模式，自动处理依赖安装和构建流程。
#
# 使用方法:
#   .\build_windows.ps1 [-BuildType Debug|Release] [-EnableCuda] [-EnableQt] [-Clean]
#
# @author Kelin
# @version 2.0
# @date 2025-09-22
#]]

param(
    [string]$BuildType = "Release",
    [switch]$EnableCuda = $false,
    [switch]$EnableQt = $false,
    [switch]$Clean = $false,
    [string]$VcpkgRoot = "D:\Software\vcpkg"
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-Success { param($msg) Write-Host $msg -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host $msg -ForegroundColor Yellow }
function Write-Error { param($msg) Write-Host $msg -ForegroundColor Red }
function Write-Info { param($msg) Write-Host $msg -ForegroundColor Cyan }

Write-Info "=== Radar MVP vcpkg 构建脚本 ==="

# 验证 vcpkg 安装
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg 未找到: $vcpkgExe"
    Write-Info "请设置正确的 VcpkgRoot 参数"
    exit 1
}

Write-Success "找到 vcpkg: $vcpkgExe"

# 获取项目根目录
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

Write-Info "项目目录: $projectRoot"

# 设置环境变量
$env:VCPKG_ROOT = $VcpkgRoot

# 清理构建目录
if ($Clean -and (Test-Path "build")) {
    Write-Info "清理构建目录..."
    Remove-Item "build" -Recurse -Force
    Write-Success "构建目录已清理"
}

# 创建构建目录
$buildDir = "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# 检查是否为manifest模式
$isManifestMode = Test-Path "vcpkg.json"

Write-Info "开始 vcpkg 包安装..."

if ($isManifestMode) {
    Write-Info "检测到 vcpkg.json，使用 manifest 模式安装所有依赖..."
    & $vcpkgExe install --triplet x64-windows
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Manifest 模式包安装失败"
        Write-Info "请检查 vcpkg.json 和 vcpkg-configuration.json 配置"
        exit 1
    }

    # 安装可选feature包
    if ($EnableQt) {
        Write-Info "安装 Qt 功能..."
        & $vcpkgExe install --feature qt --triplet x64-windows
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Qt 功能安装失败，将禁用 Qt 支持"
            $EnableQt = $false
        }
    }
} else {
    Write-Warning "未找到 vcpkg.json，请确保项目已配置为 manifest 模式"
    Write-Info "正在尝试经典模式安装基础包..."

    # 安装基础包（经典模式）
    $basePackages = @("spdlog", "yaml-cpp", "gtest", "boost-system", "boost-filesystem", "boost-thread")

    foreach ($package in $basePackages) {
        Write-Info "安装包: $package"
        & $vcpkgExe install $package --triplet x64-windows --classic
        if ($LASTEXITCODE -ne 0) {
            Write-Error "包安装失败: $package"
            exit 1
        }
    }

    # 安装可选包
    if ($EnableQt) {
        Write-Info "安装 Qt 包..."
        & $vcpkgExe install qt6-base qt6-charts --triplet x64-windows --classic
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Qt 包安装失败，将禁用 Qt 支持"
            $EnableQt = $false
        }
    }
}

Write-Success "所有包安装完成"

Write-Info "开始 CMake 配置..."

# 构建 CMake 参数
$cmakeArgs = @(
    "-B", $buildDir,
    "-S", ".",
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake",
    "-DBUILD_TESTS=ON"
)

if ($EnableCuda) {
    $cmakeArgs += "-DENABLE_CUDA=ON"
}

if ($EnableQt) {
    $cmakeArgs += "-DENABLE_QT=ON"
}

# 执行 CMake 配置
& cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake 配置失败"
    exit 1
}

Write-Success "CMake 配置完成"

Write-Info "开始编译..."

# 执行编译
& cmake --build $buildDir --config $BuildType --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Error "编译失败"
    exit 1
}

Write-Success "编译完成"

Write-Info "运行测试..."

# 运行测试
Set-Location $buildDir
& ctest --output-on-failure --build-config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Warning "部分测试失败"
} else {
    Write-Success "所有测试通过"
}

Set-Location $projectRoot

Write-Success "=== 构建完成 ==="
Write-Info "可执行文件位置: $buildDir\bin\$BuildType\radar_mvp.exe"
