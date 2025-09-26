#[[
# @file migrate_to_vcpkg.ps1
# @brief 项目迁移到 vcpkg 的辅助脚本
#
# 该脚本帮助将项目从手动依赖管理迁移到 vcpkg manifest 模式，
# 包括备份旧的第三方库、清理构建缓存、验证配置等。
#
# 使用方法:
#   .\migrate_to_vcpkg.ps1 [-VcpkgRoot "D:\Software\vcpkg"] [-SkipBackup]
#
# @author Klein
# @version 1.0
# @date 2025-09-22
#]]

param(
    [string]$VcpkgRoot = "D:\Software\vcpkg",
    [switch]$SkipBackup = $false
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-Success { param($msg) Write-Host $msg -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host $msg -ForegroundColor Yellow }
function Write-Error { param($msg) Write-Host $msg -ForegroundColor Red }
function Write-Info { param($msg) Write-Host $msg -ForegroundColor Cyan }

Write-Info "=== 项目 vcpkg 迁移脚本 ==="

$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

Write-Info "项目目录: $projectRoot"

# 1. 验证 vcpkg 安装
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg 未找到: $vcpkgExe"
    Write-Info "请设置正确的 VcpkgRoot 参数或安装 vcpkg"
    exit 1
}

Write-Success "找到 vcpkg: $vcpkgExe"

# 2. 备份当前第三方库目录
if (-not $SkipBackup -and (Test-Path "third_party")) {
    Write-Info "备份当前第三方库目录..."
    $backupName = "third_party_backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    Rename-Item "third_party" $backupName
    Write-Success "备份完成: $backupName"
    Write-Warning "如果迁移成功，可以安全删除备份目录"
}

# 3. 清理构建目录和缓存
$cleanupDirs = @("build", "vcpkg_installed", ".vcpkg-root")
foreach ($dir in $cleanupDirs) {
    if (Test-Path $dir) {
        Write-Info "清理目录: $dir"
        Remove-Item $dir -Recurse -Force
    }
}

# 4. 验证必要的配置文件
$requiredFiles = @("vcpkg.json", "vcpkg-configuration.json")
$missingFiles = @()

foreach ($file in $requiredFiles) {
    if (-not (Test-Path $file)) {
        $missingFiles += $file
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Error "缺少必要的配置文件:"
    foreach ($file in $missingFiles) {
        Write-Info "  - $file"
    }
    Write-Info "请先创建这些文件，然后重新运行迁移脚本"
    exit 1
}

Write-Success "找到所有必要的配置文件"

# 5. 设置环境变量
$env:VCPKG_ROOT = $VcpkgRoot
Write-Info "设置 VCPKG_ROOT: $VcpkgRoot"

# 6. 验证 vcpkg 配置
Write-Info "验证 vcpkg manifest 配置..."
try {
    $manifestContent = Get-Content "vcpkg.json" | ConvertFrom-Json
    Write-Success "vcpkg.json 格式正确"
    Write-Info "  项目名称: $($manifestContent.name)"
    Write-Info "  依赖包数量: $($manifestContent.dependencies.Count)"

    if ($manifestContent.features) {
        $featureNames = $manifestContent.features.PSObject.Properties.Name -join ", "
        Write-Info "  可选功能: $featureNames"
    }
}
catch {
    Write-Error "vcpkg.json 格式错误: $($_.Exception.Message)"
    exit 1
}

try {
    $configContent = Get-Content "vcpkg-configuration.json" | ConvertFrom-Json
    Write-Success "vcpkg-configuration.json 格式正确"
    Write-Info "  注册表: $($configContent.'default-registry'.repository)"
    Write-Info "  Baseline: $($configContent.'default-registry'.baseline)"
}
catch {
    Write-Error "vcpkg-configuration.json 格式错误: $($_.Exception.Message)"
    exit 1
}

# 7. 运行初始构建测试
Write-Info "开始测试构建..."
$buildScript = Join-Path $PSScriptRoot "build_windows.ps1"

if (Test-Path $buildScript) {
    Write-Info "运行构建测试..."
    try {
        & $buildScript -BuildType Debug -Clean

        if ($LASTEXITCODE -eq 0) {
            Write-Success "=== 迁移成功! ==="
            Write-Success "项目已成功迁移到 vcpkg manifest 模式"

            if (-not $SkipBackup -and (Test-Path "third_party_backup_*")) {
                Write-Info "现在可以安全删除备份目录:"
                Get-ChildItem -Directory -Name "third_party_backup_*" | ForEach-Object {
                    Write-Info "  - $_"
                }
            }

            Write-Info "后续操作建议:"
            Write-Info "  1. 提交更改到版本控制系统"
            Write-Info "  2. 更新 README.md 中的构建说明"
            Write-Info "  3. 通知团队成员新的构建流程"
        } else {
            Write-Error "构建测试失败，请检查错误信息"
            Write-Info "可以尝试以下解决方案:"
            Write-Info "  1. 检查 vcpkg.json 中的包名和版本"
            Write-Info "  2. 更新 vcpkg baseline: .\scripts\update_vcpkg_baseline.ps1"
            Write-Info "  3. 检查 CMakeLists.txt 中的包引用"
        }
    }
    catch {
        Write-Error "构建脚本执行失败: $($_.Exception.Message)"
        exit 1
    }
} else {
    Write-Warning "构建脚本不存在: $buildScript"
    Write-Info "请手动验证 vcpkg 安装是否成功"
}

Write-Info "=== 迁移过程完成 ==="
