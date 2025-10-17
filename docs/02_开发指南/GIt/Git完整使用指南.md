# Git完整使用指南

- **标题**: Git版本控制系统完整教程
- **当前版本**: v1.0
- **负责人**: Klein

---

## 📚 Git基础概念

### 什么是Git？
Git是一个**分布式版本控制系统**，用于追踪文件变化、管理代码版本、支持团队协作。

### 核心概念理解

#### 1. 仓库（Repository）
```
本地仓库结构：
d:\work\Radar\
├── .git/           # Git管理目录（隐藏）
├── src/            # 你的项目文件
├── docs/
└── README.md
```

#### 2. 三个区域
```
工作目录(Working Directory) → 暂存区(Staging Area) → 本地仓库(Local Repository) → 远程仓库(Remote Repository)
     ↓                        ↓                       ↓                         ↓
  修改文件                   git add                git commit              git push
```

#### 3. 文件状态
- **Untracked**: 新建文件，Git未跟踪
- **Modified**: 已修改，但未添加到暂存区
- **Staged**: 已添加到暂存区，等待提交
- **Committed**: 已提交到本地仓库

### Git工作流程图
```
1. 编辑文件 → 2. git add → 3. git commit → 4. git push
   ↑_______________________________________________|
                    5. git pull（获取最新版本）
```

---

## ⚙️ 基本Git操作

### 环境配置（首次使用必做）

```bash
# 设置用户信息（全局配置）
git config --global user.name "你的姓名"
git config --global user.email "your.email@example.com"

# 查看配置
git config --list

# 设置默认编辑器（可选）
git config --global core.editor "code --wait"  # 使用VS Code
```

### 创建和初始化仓库

```bash
# 方法1：创建新仓库
cd d:\work\Radar
git init

# 方法2：克隆远程仓库
git clone https://github.com/username/Radar.git
cd Radar
```

### 基本文件操作

#### 查看状态
```bash
# 查看仓库状态
git status

# 简化状态显示
git status -s
```

#### 添加文件到暂存区
```bash
# 添加单个文件
git add src/main.cpp

# 添加多个文件
git add src/main.cpp include/types.h

# 添加整个目录
git add src/

# 添加所有修改的文件
git add .

# 添加所有文件（包括删除的）
git add -A
```

#### 提交变更
```bash
# 基本提交
git commit -m "feat: 添加数据接收模块"

# 详细提交信息
git commit -m "feat: 实现DataReceiver模块

- 添加硬件接口抽象
- 实现数据缓冲机制
- 支持实时数据流处理
- 添加相应的单元测试"

# 提交所有已跟踪的修改文件（跳过git add）
git commit -am "fix: 修复内存泄漏问题"
```

#### 查看历史记录
```bash
# 查看提交历史
git log

# 简化显示
git log --oneline

# 图形化显示分支
git log --graph --oneline --all

# 查看最近3次提交
git log -3

# 查看某个文件的历史
git log src/main.cpp
```

### 撤销操作

```bash
# 撤销工作目录的修改（危险操作！）
git checkout -- src/main.cpp

# 撤销暂存区的文件（不影响工作目录）
git reset HEAD src/main.cpp

# 撤销最后一次提交（保留修改在工作目录）
git reset --soft HEAD~1

# 撤销最后一次提交（修改回到暂存区）
git reset HEAD~1

# 完全撤销最后一次提交（危险操作！）
git reset --hard HEAD~1
```

---

## 🌿 分支管理详解

### 为什么要使用分支？

#### 开发场景示例
```
主分支（main）：     ●─────●─────●─────●  (稳定版本)
                    │     │     │     │
功能分支（feature）： └──●──●──●──┘     │  (开发新功能)
                           │           │
修复分支（hotfix）：        └──●──●──●──┘  (紧急修复)
```

#### 雷达项目分支策略
```
main（主分支）
├── develop（开发分支）
│   ├── feature/data-receiver    # 数据接收模块
│   ├── feature/data-processor   # 数据处理模块
│   ├── feature/gpu-acceleration # GPU加速功能
│   └── feature/real-time-viz    # 实时可视化
├── release/v1.0                 # 发布分支
└── hotfix/memory-leak          # 紧急修复
```

### 分支基本操作

#### 创建和切换分支
```bash
# 查看所有分支
git branch

# 查看远程分支
git branch -r

# 查看所有分支（本地+远程）
git branch -a

# 创建新分支
git branch feature/data-receiver

# 切换到分支
git checkout feature/data-receiver

# 创建并切换分支（一步完成）
git checkout -b feature/data-processor

# 新语法（Git 2.23+）
git switch feature/data-processor
git switch -c feature/new-feature
```

#### 分支合并
```bash
# 切换到目标分支（通常是main或develop）
git checkout main

# 合并功能分支
git merge feature/data-receiver

# 无快进合并（保留分支历史）
git merge --no-ff feature/data-receiver
```

#### 分支删除
```bash
# 删除已合并的分支
git branch -d feature/data-receiver

# 强制删除分支（未合并也删除）
git branch -D feature/abandoned-feature

# 删除远程分支
git push origin --delete feature/data-receiver
```

### 实际开发流程示例

#### 场景：开发GPU加速功能
```bash
# 1. 从最新的develop分支创建功能分支
git checkout develop
git pull origin develop
git checkout -b feature/gpu-acceleration

# 2. 开发功能
# 编辑文件 src/modules/data_processor/gpu_processor.cpp
git add src/modules/data_processor/
git commit -m "feat: 添加GPU处理基础框架"

# 继续开发...
git add .
git commit -m "feat: 实现CUDA内核函数"

# 3. 推送到远程
git push -u origin feature/gpu-acceleration

# 4. 开发完成，合并回develop
git checkout develop
git pull origin develop  # 获取最新代码
git merge feature/gpu-acceleration
git push origin develop

# 5. 删除功能分支
git branch -d feature/gpu-acceleration
git push origin --delete feature/gpu-acceleration
```

---

## 🌐 远程仓库操作

### 远程仓库管理

```bash
# 查看远程仓库
git remote -v

# 添加远程仓库
git remote add origin https://github.com/username/Radar.git

# 修改远程仓库URL
git remote set-url origin https://github.com/username/new-repo.git

# 删除远程仓库
git remote remove origin
```

### 推送和拉取

```bash
# 推送到远程仓库
git push origin main

# 首次推送并设置上游分支
git push -u origin main

# 推送所有分支
git push --all origin

# 推送标签
git push --tags

# 从远程仓库拉取
git pull origin main

# 等价于：
git fetch origin
git merge origin/main

# 强制拉取（覆盖本地修改）
git fetch origin
git reset --hard origin/main
```

### 处理冲突

#### 冲突产生场景
```
你的修改：     A─────B─────C（你的feature分支）
              │
远程修改：     A─────D─────E（别人的修改已推送）
```

#### 解决冲突步骤
```bash
# 1. 拉取最新代码（会产生冲突）
git pull origin main

# 2. Git会标识冲突文件
# 编辑冲突文件，选择保留的代码：
# <<<<<<< HEAD
# 你的代码
# =======
# 别人的代码
# >>>>>>> commit-hash

# 3. 解决冲突后，添加到暂存区
git add conflicted-file.cpp

# 4. 提交合并
git commit -m "resolve: 解决数据处理模块冲突"

# 5. 推送
git push origin main
```

---

## 🤝 协作工作流

### Git Flow工作流（推荐用于雷达项目）

#### 分支类型说明
```
main分支：      生产环境代码，永远稳定
develop分支：   开发主分支，功能集成
feature分支：   功能开发分支
release分支：   发布准备分支
hotfix分支：    紧急修复分支
```

#### 实际工作流程

##### 1. 开发新功能
```bash
# 从develop创建功能分支
git checkout develop
git pull origin develop
git checkout -b feature/real-time-visualization

# 开发功能...
git add .
git commit -m "feat: 实现实时数据可视化组件"

# 推送功能分支
git push -u origin feature/real-time-visualization

# 创建Pull Request（GitHub）或Merge Request（GitLab）
```

##### 2. 发布版本
```bash
# 从develop创建发布分支
git checkout develop
git checkout -b release/v1.0.0

# 修复发布相关的bug
git add .
git commit -m "fix: 修复版本号显示问题"

# 合并到main
git checkout main
git merge release/v1.0.0
git tag -a v1.0.0 -m "Release version 1.0.0"

# 合并回develop
git checkout develop
git merge release/v1.0.0

# 删除发布分支
git branch -d release/v1.0.0
```

##### 3. 紧急修复
```bash
# 从main创建修复分支
git checkout main
git checkout -b hotfix/memory-leak-fix

# 修复问题
git add .
git commit -m "fix: 修复GPU内存泄漏问题"

# 合并到main
git checkout main
git merge hotfix/memory-leak-fix
git tag -a v1.0.1 -m "Hotfix version 1.0.1"

# 合并到develop
git checkout develop
git merge hotfix/memory-leak-fix

# 删除修复分支
git branch -d hotfix/memory-leak-fix
```

### Pull Request/Merge Request工作流

#### 代码审查流程
```bash
# 1. 创建功能分支并推送
git checkout -b feature/gpu-optimization
# ... 开发代码 ...
git push -u origin feature/gpu-optimization

# 2. 在GitHub/GitLab创建PR/MR
# 3. 代码审查和讨论
# 4. 根据反馈修改代码
git add .
git commit -m "refactor: 根据审查意见优化GPU内存管理"
git push origin feature/gpu-optimization

# 5. 审查通过后合并
```

---

## 🔧 高级Git操作

### 标签管理

```bash
# 创建轻量标签
git tag v1.0.0

# 创建注释标签
git tag -a v1.0.0 -m "Radar MVP Release 1.0.0"

# 查看标签
git tag

# 查看标签详情
git show v1.0.0

# 推送标签
git push origin v1.0.0
git push --tags

# 删除标签
git tag -d v1.0.0
git push origin --delete v1.0.0
```

### 储藏（Stash）

```bash
# 储藏当前工作
git stash

# 储藏并添加说明
git stash save "临时保存GPU优化代码"

# 查看储藏列表
git stash list

# 恢复最新储藏
git stash pop

# 恢复指定储藏
git stash apply stash@{1}

# 删除储藏
git stash drop stash@{1}

# 清空所有储藏
git stash clear
```

### 变基（Rebase）

```bash
# 交互式变基（整理提交历史）
git rebase -i HEAD~3

# 变基到另一个分支
git checkout feature/gpu-acceleration
git rebase develop

# 解决变基冲突
# 1. 解决冲突文件
# 2. git add 冲突文件
# 3. git rebase --continue
```

### 樱桃挑选（Cherry-pick）

```bash
# 将特定提交应用到当前分支
git cherry-pick commit-hash

# 应用多个提交
git cherry-pick commit1 commit2 commit3
```

---

## ⚠️ 常见问题解决

### 1. 提交信息写错了

```bash
# 修改最后一次提交信息
git commit --amend -m "正确的提交信息"

# 如果已经推送，需要强制推送
git push --force-with-lease origin branch-name
```

### 2. 提交了错误的文件

```bash
# 从暂存区移除文件
git reset HEAD unwanted-file.txt

# 从历史中完全删除文件
git filter-branch --tree-filter 'rm -f sensitive-file.txt' HEAD
```

### 3. 推送被拒绝

```bash
# 远程有新提交，需要先拉取
git pull origin main
# 解决可能的冲突后推送
git push origin main
```

### 4. 分支丢失

```bash
# 查看所有操作历史
git reflog

# 恢复丢失的分支
git checkout -b recovered-branch commit-hash
```

### 5. 合并了错误的分支

```bash
# 撤销最后一次合并
git reset --hard HEAD~1

# 如果已经推送，创建反向提交
git revert -m 1 merge-commit-hash
```

---

## 📋 最佳实践

### 提交信息规范

#### 格式
```
type(scope): description

[optional body]

[optional footer]
```

#### 类型说明
- **feat**: 新功能
- **fix**: 修复bug
- **docs**: 文档更新
- **style**: 代码格式化
- **refactor**: 重构代码
- **test**: 添加测试
- **chore**: 构建工具或辅助工具的变动

#### 示例
```bash
git commit -m "feat(data-processor): 添加GPU加速支持

- 实现CUDA内核函数用于FFT计算
- 添加GPU内存管理器
- 性能提升约300%

Closes #123"
```

### 分支命名规范

```bash
# 功能分支
feature/data-receiver
feature/gpu-acceleration
feature/real-time-viz

# 修复分支
fix/memory-leak
fix/compilation-error

# 发布分支
release/v1.0.0
release/v2.0.0-beta

# 热修复分支
hotfix/critical-security-fix
hotfix/performance-regression
```

### .gitignore文件配置

```gitignore
# 编译输出
build/
bin/
obj/
*.exe
*.dll
*.lib
*.a
*.so

# IDE文件
.vscode/
.vs/
*.vcxproj.user
*.suo

# 临时文件
*.tmp
*.log
*.bak
*~

# 系统文件
.DS_Store
Thumbs.db

# 项目特定
config/local_config.yaml
data/test_data/large_files/
third_party/downloads/
```

### 日常工作建议

#### 每日工作流程
```bash
# 1. 开始工作前
git checkout develop
git pull origin develop

# 2. 创建功能分支
git checkout -b feature/your-feature

# 3. 定期提交
git add .
git commit -m "feat: 实现基础功能框架"

# 4. 定期推送
git push -u origin feature/your-feature

# 5. 工作结束
git add .
git commit -m "feat: 完成功能实现和测试"
git push origin feature/your-feature
```

#### 团队协作建议
1. **小而频繁的提交**: 每个功能点都单独提交
2. **清晰的分支策略**: 严格按照Git Flow执行
3. **代码审查**: 所有代码通过PR进行审查
4. **同步更新**: 定期从主分支同步最新代码
5. **冲突预防**: 避免多人同时修改同一文件

---

## 🎓 进阶学习资源

### 推荐工具
- **GUI工具**: GitHub Desktop, SourceTree, GitKraken
- **VS Code插件**: GitLens, Git Graph
- **命令行增强**: Oh My Zsh (Git插件), Tig

### 学习路径
1. **基础阶段**: 掌握add, commit, push, pull
2. **分支阶段**: 熟练使用分支和合并
3. **协作阶段**: 理解工作流和代码审查
4. **高级阶段**: 掌握rebase, cherry-pick等高级操作
5. **专家阶段**: 自定义工作流和Git钩子

### 实践练习建议
1. 在测试项目中练习所有基本操作
2. 模拟团队协作场景
3. 练习解决各种冲突
4. 尝试不同的工作流模式

---

## 📚 快速参考

### 常用命令速查表

| 操作     | 命令                       |
| -------- | -------------------------- |
| 克隆仓库 | `git clone <url>`          |
| 查看状态 | `git status`               |
| 添加文件 | `git add <file>`           |
| 提交变更 | `git commit -m "message"`  |
| 推送代码 | `git push origin <branch>` |
| 拉取代码 | `git pull origin <branch>` |
| 创建分支 | `git checkout -b <branch>` |
| 切换分支 | `git checkout <branch>`    |
| 合并分支 | `git merge <branch>`       |
| 查看历史 | `git log --oneline`        |

### 紧急情况处理

| 问题             | 解决方案                                                     |
| ---------------- | ------------------------------------------------------------ |
| 提交了敏感文件   | `git reset --soft HEAD~1`                                    |
| 需要撤销本地修改 | `git checkout -- <file>`                                     |
| 推送被拒绝       | `git pull` 然后 `git push`                                   |
| 分支合并冲突     | 编辑冲突文件，`git add`，`git commit`                        |
| 误删分支         | `git reflog` 找到commit，`git checkout -b <branch> <commit>` |

---

## 变更记录

| 版本 | 日期       | 修改人       | 变更摘要            |
| ---- | ---------- | ------------ | ------------------- |
| v1.0 | 2025-01-20 | AI Assistant | 创建Git完整使用指南 |

---

这个指南将帮助你从Git新手成长为熟练用户。建议按照技能等级逐步学习，在雷达项目中实际应用这些技能。有任何问题随时询问！
