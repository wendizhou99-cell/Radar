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

    # 检查是否意外包含私人文档
    $privateFiles = git ls-files docs_private/ 2>$null
    if ($privateFiles) {
        Write-Host "⚠️  警告：检测到docs_private文件夹中的文件被跟踪！" -ForegroundColor Red
        Write-Host "这些是私人文档，不应该提交到仓库。" -ForegroundColor Yellow
        Write-Host "请先运行以下命令移除跟踪：" -ForegroundColor Cyan
        Write-Host "git rm -r --cached docs_private/" -ForegroundColor White
        return
    }

    git add .
    git commit -m "$Type($Scope): $Message"
    Write-Host "提交完成: $Type($Scope): $Message" -ForegroundColor Green
}# 同步远程仓库
function Sync-WithRemote {
    $currentBranch = git branch --show-current
    Write-Host "同步当前分支 '$currentBranch' 与远程仓库..." -ForegroundColor Yellow
    git pull origin $currentBranch
    git push origin $currentBranch
    Write-Host "同步完成！" -ForegroundColor Green
}

# 推送所有分支
function Push-AllBranches {
    Write-Host "推送所有分支到远程仓库..." -ForegroundColor Yellow
    git push --all origin
    Write-Host "所有分支推送完成！" -ForegroundColor Green
}

# 查看远程仓库状态
function Show-RemoteStatus {
    Write-Host "=== 远程仓库状态 ===" -ForegroundColor Cyan
    git remote -v
    Write-Host "`n远程分支:" -ForegroundColor Yellow
    git branch -r
}

# 检查私人文档保护状态
function Check-PrivateDocProtection {
    Write-Host "=== 私人文档保护检查 ===" -ForegroundColor Cyan

    # 检查.gitignore中是否包含docs_private
    $gitignoreContent = Get-Content .gitignore -ErrorAction SilentlyContinue
    if ($gitignoreContent -contains "docs_private/") {
        Write-Host "✅ .gitignore中已正确配置docs_private/过滤" -ForegroundColor Green
    } else {
        Write-Host "❌ .gitignore中缺少docs_private/过滤" -ForegroundColor Red
    }

    # 检查是否有私人文档被意外跟踪
    $trackedPrivateFiles = git ls-files docs_private/ 2>$null
    if ($trackedPrivateFiles) {
        Write-Host "❌ 检测到私人文档被Git跟踪！" -ForegroundColor Red
        Write-Host "被跟踪的文件数量: $($trackedPrivateFiles.Count)" -ForegroundColor Yellow
        Write-Host "请运行: git rm -r --cached docs_private/" -ForegroundColor Cyan
    } else {
        Write-Host "✅ 没有私人文档被Git跟踪" -ForegroundColor Green
    }

    # 检查本地是否存在私人文档
    if (Test-Path "docs_private") {
        $fileCount = (Get-ChildItem "docs_private" -Recurse -File).Count
        Write-Host "✅ 本地私人文档存在，包含 $fileCount 个文件" -ForegroundColor Green
    } else {
        Write-Host "⚠️  本地未发现docs_private文件夹" -ForegroundColor Yellow
    }
}

# 别名设置
Set-Alias -Name "radar-switch" -Value Switch-ToFeatureBranch
Set-Alias -Name "radar-new" -Value New-FeatureBranch
Set-Alias -Name "radar-merge" -Value Merge-FeatureToDevelop
Set-Alias -Name "radar-status" -Value Show-ProjectBranches
Set-Alias -Name "radar-commit" -Value Submit-RadarChanges
Set-Alias -Name "radar-sync" -Value Sync-WithRemote
Set-Alias -Name "radar-push-all" -Value Push-AllBranches
Set-Alias -Name "radar-remote" -Value Show-RemoteStatus
Set-Alias -Name "radar-check-private" -Value Check-PrivateDocProtection

Write-Host "Radar项目Git管理脚本已加载！" -ForegroundColor Green
Write-Host "可用命令:" -ForegroundColor Cyan
Write-Host "  radar-switch <branch-name>  - 切换到功能分支" -ForegroundColor White
Write-Host "  radar-new <branch-name>     - 创建新功能分支" -ForegroundColor White
Write-Host "  radar-merge <branch-name>   - 合并功能分支" -ForegroundColor White
Write-Host "  radar-status                - 显示项目状态" -ForegroundColor White
Write-Host "  radar-commit                - 标准化提交(含私人文档保护检查)" -ForegroundColor White
Write-Host "  radar-sync                  - 同步当前分支与远程" -ForegroundColor White
Write-Host "  radar-push-all              - 推送所有分支到远程" -ForegroundColor White
Write-Host "  radar-remote                - 查看远程仓库状态" -ForegroundColor White
Write-Host "  radar-check-private         - 检查私人文档保护状态" -ForegroundColor White
