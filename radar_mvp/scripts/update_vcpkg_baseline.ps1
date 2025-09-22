#[[
# @file update_vcpkg_baseline.ps1
# @brief 自动更新 vcpkg baseline 到最新版本的脚本
#
# 该脚本从 GitHub API 获取最新的 vcpkg 主分支 commit SHA，
# 并更新项目的 vcpkg-configuration.json 文件中的 baseline。
#
# 使用方法:
#   .\update_vcpkg_baseline.ps1 [-VcpkgRoot "D:\Software\vcpkg"]
#
# @author Kelin
# @version 1.0
# @date 2025-09-22
#]]

param(
    [string]$VcpkgRoot = "D:\Software\vcpkg"
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-Success { param($msg) Write-Host $msg -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host $msg -ForegroundColor Yellow }
function Write-Error { param($msg) Write-Host $msg -ForegroundColor Red }
function Write-Info { param($msg) Write-Host $msg -ForegroundColor Cyan }

# 获取最新的vcpkg baseline
function Get-LatestVcpkgBaseline {
    param([string]$VcpkgPath)

    try {
        Write-Info "从 GitHub API 获取最新 baseline..."
        $response = Invoke-RestMethod -Uri "https://api.github.com/repos/Microsoft/vcpkg/commits/master" -Headers @{
            "User-Agent" = "RadarMVP-Build-Script"
        }
        Write-Success "获取到最新 baseline: $($response.sha)"
        return $response.sha
    }
    catch {
        Write-Warning "无法从 GitHub API 获取最新baseline，尝试使用本地vcpkg仓库..."
        if (Test-Path "$VcpkgPath\.git") {
            Push-Location $VcpkgPath
            try {
                $sha = git rev-parse HEAD
                Write-Success "从本地仓库获取 baseline: $sha"
                return $sha
            }
            catch {
                Write-Error "无法从本地仓库获取 baseline"
                return $null
            }
            finally {
                Pop-Location
            }
        }
        Write-Error "本地 vcpkg 仓库不存在: $VcpkgPath"
        return $null
    }
}

Write-Info "=== vcpkg Baseline 更新脚本 ==="

$projectRoot = Split-Path -Parent $PSScriptRoot
$configFile = "$projectRoot\vcpkg-configuration.json"

# 验证配置文件存在
if (-not (Test-Path $configFile)) {
    Write-Error "vcpkg-configuration.json 文件不存在: $configFile"
    exit 1
}

Write-Info "配置文件: $configFile"

# 获取新的 baseline
$newBaseline = Get-LatestVcpkgBaseline -VcpkgPath $VcpkgRoot

if (-not $newBaseline) {
    Write-Error "无法获取新的 baseline"
    exit 1
}

# 更新配置文件
try {
    $config = Get-Content $configFile | ConvertFrom-Json
    $oldBaseline = $config.'default-registry'.baseline

    if ($oldBaseline -eq $newBaseline) {
        Write-Success "Baseline 已是最新版本: $newBaseline"
        exit 0
    }

    $config.'default-registry'.baseline = $newBaseline
    $config | ConvertTo-Json -Depth 10 | Set-Content $configFile -Encoding UTF8

    Write-Success "已更新 baseline:"
    Write-Info "  旧版本: $oldBaseline"
    Write-Info "  新版本: $newBaseline"

    Write-Warning "建议在更新后运行构建测试以验证兼容性"
}
catch {
    Write-Error "更新配置文件失败: $($_.Exception.Message)"
    exit 1
}

Write-Success "=== Baseline 更新完成 ==="
