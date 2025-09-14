# Radar项目Git分支管理脚本
# 基于docs_private/04_技术栈配置/Git完整使用指南.md

# 快速切换到各功能分支
function Switch-ToFeatureBranch {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateSet("data-receiver", "data-processor", "gpu-acceleration", "real-time-viz")]
        [string]$FeatureName
    )
    
    git checkout "feature/$FeatureName"
}

# 快速创建新功能分支
function New-FeatureBranch {
    param(
        [Parameter(Mandatory=$true)]
        [string]$FeatureName
    )
    
    git checkout develop
    git checkout -b "feature/$FeatureName"
    Write-Host "创建并切换到分支: feature/$FeatureName" -ForegroundColor Green
}

# 合并功能分支到develop
function Merge-FeatureToDevelop {
    param(
        [Parameter(Mandatory=$true)]
        [string]$FeatureName
    )
    
    git checkout develop
    git merge "feature/$FeatureName" --no-ff
    Write-Host "已将 feature/$FeatureName 合并到 develop 分支" -ForegroundColor Green
}

# 删除已合并的功能分支
function Remove-MergedFeature {
    param(
        [Parameter(Mandatory=$true)]
        [string]$FeatureName
    )
    
    git branch -d "feature/$FeatureName"
    Write-Host "已删除分支: feature/$FeatureName" -ForegroundColor Red
}

# 显示项目分支状态
function Show-ProjectBranches {
    Write-Host "=== Radar项目分支状态 ===" -ForegroundColor Cyan
    Write-Host "当前分支:" -ForegroundColor Yellow
    git branch --show-current
    Write-Host "`n所有分支:" -ForegroundColor Yellow
    git branch -a
    Write-Host "`n最近提交:" -ForegroundColor Yellow
    git log --oneline -5
}

# 标准提交流程
function Submit-RadarChanges {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateSet("feat", "fix", "docs", "style", "refactor", "perf", "test", "chore")]
        [string]$Type,
        
        [Parameter(Mandatory=$true)]
        [ValidateSet("data-receiver", "data-processor", "gpu-acceleration", "real-time-viz", "task-scheduler", "common", "config", "build")]
        [string]$Scope,
        
        [Parameter(Mandatory=$true)]
        [string]$Message
    )
    
    git add .
    git commit -m "$Type($Scope): $Message"
    Write-Host "提交完成: $Type($Scope): $Message" -ForegroundColor Green
}

# 别名设置
Set-Alias -Name "radar-switch" -Value Switch-ToFeatureBranch
Set-Alias -Name "radar-new" -Value New-FeatureBranch
Set-Alias -Name "radar-merge" -Value Merge-FeatureToDevelop
Set-Alias -Name "radar-status" -Value Show-ProjectBranches
Set-Alias -Name "radar-commit" -Value Submit-RadarChanges

Write-Host "Radar项目Git管理脚本已加载！" -ForegroundColor Green
Write-Host "可用命令:" -ForegroundColor Cyan
Write-Host "  radar-switch <branch-name>  - 切换到功能分支" -ForegroundColor White
Write-Host "  radar-new <branch-name>     - 创建新功能分支" -ForegroundColor White  
Write-Host "  radar-merge <branch-name>   - 合并功能分支" -ForegroundColor White
Write-Host "  radar-status                - 显示项目状态" -ForegroundColor White
Write-Host "  radar-commit                - 标准化提交" -ForegroundColor White