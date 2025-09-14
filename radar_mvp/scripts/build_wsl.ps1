#[[
# @file build_wsl.ps1
# @brief Windows环境下通过WSL构建Linux版本的PowerShell脚本
#
# 该脚本在Windows环境中调用WSL来构建Linux版本的雷达系统，
# 自动处理路径转换、权限设置和构建流程。
#
# 使用方法:
#   .\build_wsl.ps1 [-BuildType Debug|Release] [-EnableCuda] [-Clean] [-DisableTests]
#
# @author Kelin
# @version 1.0
# @date 2025-09-13
#]]

param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",

    [switch]$EnableCuda,

    [switch]$Clean,

    [switch]$DisableTests,

    [switch]$Help
)

# 颜色输出函数
function Write-ColorMessage {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host "[BUILD] $Message" -ForegroundColor $Color
}

function Write-Info {
    param([string]$Message)
    Write-ColorMessage $Message "Cyan"
}

function Write-Success {
    param([string]$Message)
    Write-ColorMessage $Message "Green"
}

function Write-Warning {
    param([string]$Message)
    Write-ColorMessage $Message "Yellow"
}

function Write-Error {
    param([string]$Message)
    Write-ColorMessage $Message "Red"
}

# 显示帮助信息
if ($Help) {
    Write-Host @"
Radar MVP 系统 WSL 构建脚本

用法: .\build_wsl.ps1 [参数...]

参数:
  -BuildType <Debug|Release>  构建类型 (默认: Release)
  -EnableCuda                 启用CUDA支持
  -Clean                      清理构建目录
  -DisableTests              禁用测试构建
  -Help                      显示此帮助信息

示例:
  .\build_wsl.ps1                           # 默认Release构建
  .\build_wsl.ps1 -BuildType Debug          # Debug构建
  .\build_wsl.ps1 -EnableCuda -Clean        # 启用CUDA并清理构建
"@
    exit 0
}

Write-Info "=== Radar MVP 系统 WSL 构建脚本 ==="
Write-Info "构建类型: $BuildType"
Write-Info "CUDA支持: $(if ($EnableCuda) { '启用' } else { '禁用' })"
Write-Info "清理构建: $(if ($Clean) { '是' } else { '否' })"
Write-Info "构建测试: $(if ($DisableTests) { '禁用' } else { '启用' })"
Write-Info "======================================="

# 检查WSL是否可用
Write-Info "检查WSL环境..."
try {
    $wslVersion = wsl --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "WSL命令执行失败"
    }
    Write-Success "WSL环境可用"
} catch {
    Write-Error "WSL未安装或未正确配置"
    Write-Info "请安装WSL 2并配置Linux发行版:"
    Write-Info "1. 启用Windows功能: wsl --install"
    Write-Info "2. 重启计算机"
    Write-Info "3. 安装Linux发行版 (建议Ubuntu 22.04)"
    exit 1
}

# 检查当前目录是否为项目根目录
$currentDir = Get-Location
$projectMarkers = @("CMakeLists.txt", "radar_mvp", "include", "src")
$isProjectRoot = $true

foreach ($marker in $projectMarkers) {
    if (-not (Test-Path (Join-Path $currentDir $marker))) {
        $isProjectRoot = $false
        break
    }
}

if (-not $isProjectRoot) {
    Write-Error "请在项目根目录下运行此脚本"
    Write-Info "项目根目录应包含: CMakeLists.txt, radar_mvp/, include/, src/"
    exit 1
}

# 获取项目在WSL中的路径
Write-Info "转换项目路径到WSL格式..."
$windowsPath = $currentDir.Path
$wslPath = wsl wslpath -a "`"$windowsPath`""

if ($LASTEXITCODE -ne 0) {
    Write-Error "路径转换失败"
    exit 1
}

Write-Info "Windows路径: $windowsPath"
Write-Info "WSL路径: $wslPath"

# 检查构建脚本是否存在
$buildScriptPath = Join-Path $currentDir "build_linux.sh"
if (-not (Test-Path $buildScriptPath)) {
    Write-Error "构建脚本不存在: $buildScriptPath"
    exit 1
}

# 在WSL中设置脚本执行权限
Write-Info "设置脚本执行权限..."
wsl chmod +x "`"$wslPath/build_linux.sh`""

if ($LASTEXITCODE -ne 0) {
    Write-Warning "设置执行权限失败，尝试继续..."
}

# 构建命令行参数
$wslArgs = @()
$wslArgs += $BuildType

if ($EnableCuda) {
    $wslArgs += "--enable-cuda"
}

if ($Clean) {
    $wslArgs += "--clean"
}

if ($DisableTests) {
    $wslArgs += "--disable-tests"
}

$wslArgsString = $wslArgs -join " "

# 在WSL中执行构建
Write-Info "开始WSL构建..."
Write-Info "执行命令: cd `"$wslPath`" && ./build_linux.sh $wslArgsString"

$wslCommand = "cd `"$wslPath`" && ./build_linux.sh $wslArgsString"
wsl bash -c $wslCommand

$buildResult = $LASTEXITCODE

if ($buildResult -eq 0) {
    Write-Success "构建成功完成!"

    # 显示生成的文件信息
    $binPath = Join-Path $currentDir "build\bin\radar_mvp"
    if (Test-Path $binPath) {
        $fileInfo = Get-Item $binPath
        Write-Info "可执行文件: $($fileInfo.FullName)"
        Write-Info "文件大小: $([math]::Round($fileInfo.Length / 1MB, 2)) MB"
        Write-Info "修改时间: $($fileInfo.LastWriteTime)"
    }

    Write-Info "构建产物位置:"
    Write-Info "  二进制文件: build/bin/"
    Write-Info "  库文件: build/lib/"
    Write-Info "  配置文件: configs/"

} else {
    Write-Error "构建失败，退出码: $buildResult"
    Write-Info "请检查上述错误信息并修复后重试"
    exit $buildResult
}

Write-Success "=== WSL 构建完成 ==="
