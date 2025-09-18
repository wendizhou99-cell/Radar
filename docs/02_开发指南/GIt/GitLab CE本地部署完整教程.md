# GitLab CE 本地部署完整教程（免费版）


**最后更新**：2025年9月18日
**版本**：v1.0
**适用于**：GitLab CE 15.0+
**作者**：Kelin

> **适用人群**：管理员
> **目标**：在局域网内搭建类似 GitHub 的代码协作平台
> **费用**：完全免费

---

## 📋 目录

- [1. 什么是 GitLab CE？](#1-什么是-gitlab-ce)
- [2. 为什么要本地部署？](#2-为什么要本地部署)
- [3. 部署前准备](#3-部署前准备)
- [4. 方案选择](#4-方案选择)
- [5. Docker 方式部署（推荐）](#5-docker-方式部署推荐)
- [6. Ubuntu 包安装方式](#6-ubuntu-包安装方式)
- [7. 首次配置](#7-首次配置)
- [8. 团队使用指南](#8-团队使用指南)
- [9. 备份与维护](#9-备份与维护)
- [10. 常见问题解决](#10-常见问题解决)

---

## 1. 什么是 GitLab CE？

### 简单理解

**GitLab CE（Community Edition）** 就是一个"私有的 GitHub"，可以部署在你们公司内部的服务器上。

- **CE = Community Edition（社区版）**：完全免费
- **本质**：让团队在内网协作开发代码，支持 Pull Request（GitLab 叫 Merge Request）
- **功能**：代码托管、代码审查、CI/CD、项目管理

### 与 GitHub 的对比

| 功能         | GitHub                | GitLab CE（本地部署） |
| ------------ | --------------------- | --------------------- |
| 代码托管     | ✅                     | ✅                     |
| Pull Request | ✅                     | ✅（叫 Merge Request） |
| 代码审查     | ✅                     | ✅                     |
| CI/CD        | ✅                     | ✅                     |
| 数据控制     | ❌（在 GitHub 服务器） | ✅（在你的服务器）     |
| 网络要求     | 需要外网              | 只需内网              |
| 费用         | 私有仓库收费          | 完全免费              |

---

## 2. 为什么要本地部署？

### 安全需求
- **数据不出公司**：所有代码都在内网，符合保密要求
- **访问控制**：只有公司内网能访问，外部无法攻击
- **审计要求**：满足企业安全审计要求

### 成本优势
- **免费使用**：GitLab CE 完全开源免费
- **无人数限制**：不像 GitHub 按用户数收费
- **存储不限**：只受服务器硬盘限制

### 功能完整
- **完整的 Git 工作流**：支持所有 Git 操作
- **Web 界面**：类似 GitHub 的友好界面
- **团队协作**：支持代码审查、项目管理

---

## 3. 部署前准备

### 3.1 硬件要求

#### 最低配置（小团队 3-5 人）
```yaml
CPU: 2 核心
内存: 4GB RAM
硬盘: 50GB 可用空间
网络: 内网固定 IP
```

#### 推荐配置（中型团队 10+ 人）
```yaml
CPU: 4 核心
内存: 8GB RAM（或更多）
硬盘: 200GB SSD
网络: 千兆内网，固定 IP
```

#### 服务器类型选择
- **物理服务器**：性能最好，适合大团队
- **虚拟机**：VMware、Hyper-V 等，灵活性好
- **云服务器**：阿里云、腾讯云等（内网版本）

### 3.2 操作系统要求

#### 推荐系统（按优先级）
1. **Ubuntu 22.04 LTS**（最推荐，兼容性最好）
2. **CentOS 8 / Rocky Linux 8**
3. **Windows Server 2019+**（通过 Docker）

#### 为什么推荐 Ubuntu？
- GitLab 官方主要测试平台
- 安装简单，文档齐全
- 包管理器方便安装依赖

### 3.3 网络规划

#### IP 地址规划
```bash
# 示例内网规划
服务器 IP: 192.168.1.10
域名设置: gitlab.company.local
端口规划:
  - HTTP: 80 (可选)
  - HTTPS: 443 (推荐)
  - SSH: 22 (Git 推拉代码用)
```

#### DNS 设置（二选一）

**方案 A：内网 DNS 服务器**
```bash
# 在 DNS 服务器添加记录
gitlab.company.local A 192.168.1.10
```

**方案 B：修改每台电脑的 hosts 文件**

Windows：编辑 `C:\Windows\System32\drivers\etc\hosts`
```
192.168.1.10 gitlab.company.local
```

Linux/Mac：编辑 `/etc/hosts`
```
192.168.1.10 gitlab.company.local
```

### 3.4 软件依赖检查

运行以下命令检查服务器是否满足要求：

```bash
# 检查内存
free -h
# 应该显示至少 4GB

# 检查硬盘空间
df -h
# 应该有至少 50GB 可用

# 检查网络连通性
ping 192.168.1.1
# 应该能 ping 通网关

# 检查端口是否被占用
netstat -tlnp | grep -E ':80|:443|:22'
# 不应该有其他程序占用这些端口
```

---

## 4. 方案选择

### 4.1 部署方案对比

| 方案         | 难度  | 优点               | 缺点            | 适合场景       |
| ------------ | ----- | ------------------ | --------------- | -------------- |
| **Docker**   | ⭐⭐    | 部署快速、容易升级 | 需要学习 Docker | 推荐：快速上手 |
| **包安装**   | ⭐⭐⭐   | 性能好、集成度高   | 升级相对复杂    | 推荐：长期使用 |
| **源码编译** | ⭐⭐⭐⭐⭐ | 定制性强           | 极其复杂        | 不推荐         |

### 4.2 推荐选择

- **初学者 / 快速验证**：选择 Docker 方式
- **生产环境 / 长期使用**：选择 Ubuntu 包安装
- **Windows 服务器**：只能选择 Docker 方式

---

## 5. Docker 方式部署（推荐）

### 5.1 安装 Docker

#### Ubuntu 服务器安装 Docker

```bash
# 1. 更新系统
sudo apt update && sudo apt upgrade -y

# 2. 安装 Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# 3. 启动 Docker 服务
sudo systemctl start docker
sudo systemctl enable docker

# 4. 验证安装
sudo docker --version
# 应该显示 Docker 版本信息

# 5. 添加当前用户到 docker 组（可选）
sudo usermod -aG docker $USER
# 需要重新登录生效
```

#### Windows 服务器安装 Docker

1. 下载 Docker Desktop for Windows
2. 双击安装，重启服务器
3. 在 PowerShell 中验证：
```powershell
docker --version
```

### 5.2 创建数据目录

```bash
# 创建 GitLab 数据存储目录
sudo mkdir -p /srv/gitlab/config
sudo mkdir -p /srv/gitlab/logs
sudo mkdir -p /srv/gitlab/data

# 设置权限
sudo chmod 755 /srv/gitlab
sudo chown -R 998:998 /srv/gitlab
```

### 5.3 运行 GitLab 容器

#### 基础运行命令

```bash
sudo docker run --detach \
  --hostname gitlab.company.local \
  --publish 443:443 \
  --publish 80:80 \
  --publish 22:22 \
  --name gitlab \
  --restart always \
  --volume /srv/gitlab/config:/etc/gitlab \
  --volume /srv/gitlab/logs:/var/log/gitlab \
  --volume /srv/gitlab/data:/var/opt/gitlab \
  gitlab/gitlab-ce:latest
```

#### 参数解释

```bash
--detach              # 后台运行
--hostname            # 设置内部主机名
--publish 443:443     # 映射 HTTPS 端口
--publish 80:80       # 映射 HTTP 端口
--publish 22:22       # 映射 SSH 端口（Git 推拉代码用）
--name gitlab         # 容器名称
--restart always      # 开机自动启动
--volume              # 数据持久化（重要！）
```

### 5.4 带环境变量的高级配置

```bash
sudo docker run --detach \
  --hostname gitlab.company.local \
  --publish 443:443 \
  --publish 80:80 \
  --publish 2222:22 \
  --name gitlab \
  --restart always \
  --volume /srv/gitlab/config:/etc/gitlab \
  --volume /srv/gitlab/logs:/var/log/gitlab \
  --volume /srv/gitlab/data:/var/opt/gitlab \
  --env GITLAB_OMNIBUS_CONFIG="
    external_url 'https://gitlab.company.local';
    gitlab_rails['smtp_enable'] = false;
    gitlab_rails['time_zone'] = 'Asia/Shanghai';
    " \
  gitlab/gitlab-ce:latest
```

### 5.5 检查部署状态

```bash
# 查看容器状态
sudo docker ps

# 查看启动日志
sudo docker logs -f gitlab

# 等待启动完成（通常需要 3-5 分钟）
# 看到 "GitLab is now ready to use" 表示启动完成
```

### 5.6 首次访问

1. **等待启动完成**（重要！）
   - 第一次启动需要 3-10 分钟
   - 可以用 `sudo docker logs -f gitlab` 查看进度

2. **打开浏览器访问**
   ```
   http://gitlab.company.local
   或
   http://192.168.1.10
   ```

3. **获取初始密码**
   ```bash
   # 查看自动生成的 root 密码
   sudo docker exec -it gitlab grep 'Password:' /etc/gitlab/initial_root_password
   ```

4. **首次登录**
   - 用户名：`root`
   - 密码：上一步获取的密码

---

## 6. Ubuntu 包安装方式

### 6.1 系统准备

```bash
# 1. 更新系统
sudo apt update && sudo apt upgrade -y

# 2. 安装基础依赖
sudo apt install -y ca-certificates curl openssh-server tzdata perl

# 3. 安装邮件服务（可选，用于发送通知邮件）
sudo apt install -y postfix
# 选择 "Internet Site"，域名填写公司域名
```

### 6.2 添加 GitLab 官方仓库

```bash
# 1. 下载安装脚本
curl https://packages.gitlab.com/install/repositories/gitlab/gitlab-ce/script.deb.sh | sudo bash

# 2. 验证仓库添加成功
sudo apt update
```

### 6.3 安装 GitLab CE

```bash
# 安装 GitLab CE
# 注意：安装过程中会自动配置很多服务，需要等待较长时间
sudo EXTERNAL_URL="http://gitlab.company.local" apt-get install gitlab-ce -y
```

### 6.4 配置 GitLab

#### 编辑配置文件

```bash
# 编辑主配置文件
sudo nano /etc/gitlab/gitlab.rb
```

#### 基础配置示例

```ruby
# /etc/gitlab/gitlab.rb 文件内容

# 外部访问地址
external_url 'http://gitlab.company.local'

# 时区设置
gitlab_rails['time_zone'] = 'Asia/Shanghai'

# 禁用一些不需要的服务（节省资源）
prometheus_monitoring['enable'] = false
alertmanager['enable'] = false
node_exporter['enable'] = false
redis_exporter['enable'] = false
postgres_exporter['enable'] = false
pgbouncer_exporter['enable'] = false
gitlab_exporter['enable'] = false

# SSH 端口设置（如果需要修改）
gitlab_rails['gitlab_shell_ssh_port'] = 22
```

#### 应用配置

```bash
# 重新配置 GitLab（这个过程需要几分钟）
sudo gitlab-ctl reconfigure

# 重启 GitLab
sudo gitlab-ctl restart

# 检查服务状态
sudo gitlab-ctl status
```

### 6.5 获取初始密码

```bash
# 查看自动生成的 root 密码
sudo cat /etc/gitlab/initial_root_password
```

---

## 7. 首次配置

### 7.1 登录管理界面

1. **打开浏览器**，访问 `http://gitlab.company.local`
2. **首次登录**：
   - 用户名：`root`
   - 密码：从上一步获取的初始密码

### 7.2 修改管理员密码

1. 登录后，点击右上角头像 → **Edit profile**
2. 左侧菜单点击 **Password**
3. 输入新密码，点击 **Save password**

### 7.3 基础系统设置

#### 进入管理区域

1. 点击左侧菜单 **Admin Area**（扳手图标）
2. 或直接访问 `http://gitlab.company.local/admin`

#### 基础设置项

**1. 通用设置（Settings → General）**

```yaml
可见性设置:
  默认项目可见性: Private（私有）
  默认代码片段可见性: Private
  导入源限制: 禁用所有外部导入

注册限制:
  注册启用: 关闭（防止外部人员注册）
  或者: 仅允许指定邮箱域名注册
```

**2. 网络设置（Settings → Network）**

```yaml
推送和拉取限制:
  允许的 IP 地址: 内网 IP 段
  例如: 192.168.0.0/16, 10.0.0.0/8

传出请求:
  禁用传出请求: 启用（安全考虑）
```

**3. 邮件设置（Settings → Email）**

如果有内网邮件服务器：

```ruby
# 在 /etc/gitlab/gitlab.rb 中添加
gitlab_rails['smtp_enable'] = true
gitlab_rails['smtp_address'] = "mail.company.local"
gitlab_rails['smtp_port'] = 587
gitlab_rails['smtp_domain'] = "company.local"
gitlab_rails['gitlab_email_from'] = 'gitlab@company.local'
```

### 7.4 创建用户和组织

#### 创建组织（Group）

1. 点击顶部导航栏的 **+** → **New group**
2. 填写信息：
   ```yaml
   组织名称: radar-team
   组织路径: radar-team
   可见性: Private
   ```
3. 点击 **Create group**

#### 创建用户

1. **Admin Area** → **Users** → **New user**
2. 填写用户信息：
   ```yaml
   姓名: 张三
   用户名: zhangsan
   邮箱: zhangsan@company.local
   ```
3. 点击 **Create user**
4. 为用户设置密码：点击用户名 → **Edit** → **Password**

#### 添加用户到组织

1. 进入组织 **radar-team**
2. 左侧菜单 **Members** → **Invite members**
3. 选择用户，设置角色（推荐：Developer）
4. 点击 **Invite**

---

## 8. 团队使用指南

### 8.1 创建项目

#### 管理员创建项目

1. 进入组织 **radar-team**
2. 点击 **New project** → **Create blank project**
3. 填写项目信息：
   ```yaml
   项目名称: radar-mvp
   项目路径: radar-mvp
   可见性: Private
   初始化仓库: ✅ 勾选 "Initialize repository with a README"
   ```

#### 设置项目保护规则

1. 进入项目 → **Settings** → **Repository**
2. 展开 **Protected branches**
3. 保护 `main` 分支：
   ```yaml
   分支: main
   推送权限: Maintainers
   合并权限: Maintainers
   强制推送: 不允许
   ```

### 8.2 团队成员使用

#### 获取项目地址

在项目首页，点击 **Clone** 按钮，获取地址：

```bash
# HTTPS 方式（推荐）
https://gitlab.company.local/radar-team/radar-mvp.git

# SSH 方式（需要配置 SSH 密钥）
git@gitlab.company.local:radar-team/radar-mvp.git
```

#### 克隆项目

```bash
# 使用 HTTPS 方式克隆
git clone https://gitlab.company.local/radar-team/radar-mvp.git

# 进入项目目录
cd radar-mvp
```

#### 设置用户信息

```bash
# 设置 Git 用户信息
git config user.name "张三"
git config user.email "zhangsan@company.local"

# 或者全局设置
git config --global user.name "张三"
git config --global user.email "zhangsan@company.local"
```

### 8.3 Merge Request 工作流

#### 开发者工作流程

```bash
# 1. 确保在最新的 main 分支
git checkout main
git pull origin main

# 2. 创建功能分支
git checkout -b feature/data-parser

# 3. 进行开发工作
# ... 编辑代码 ...

# 4. 提交代码
git add .
git commit -m "feat: 实现数据解析器模块"

# 5. 推送分支
git push -u origin feature/data-parser
```

#### 创建 Merge Request

1. 推送分支后，GitLab 会显示创建 MR 的提示
2. 或者手动创建：项目页面 → **Merge requests** → **New merge request**
3. 填写 MR 信息：
   ```yaml
   源分支: feature/data-parser
   目标分支: main
   标题: 实现数据解析器模块
   描述: 详细说明修改内容和测试情况
   指派给: 项目负责人
   ```

#### 代码审查流程

1. **审查者收到通知**（如果配置了邮件）
2. **在线审查代码**：
   - 查看 **Changes** 标签页
   - 在代码行上添加评论
   - 整体评估代码质量
3. **审查决定**：
   - **Approve**：同意合并
   - **Request changes**：要求修改
4. **合并代码**：
   - 审查通过后，点击 **Merge**
   - 选择合并策略（推荐：**Merge commit**）

### 8.4 解决 HTTPS 证书问题

如果使用自签名证书，可能遇到 SSL 错误：

```bash
# 临时解决方案（不推荐用于生产）
git config --global http.sslVerify false

# 或者为特定服务器禁用 SSL 验证
git config --global http."https://gitlab.company.local".sslVerify false
```

更好的解决方案是配置内网 CA 证书，但这需要网络管理员配合。

---

## 9. 备份与维护

### 9.1 数据备份

#### Docker 方式备份

```bash
# 1. 创建备份目录
sudo mkdir -p /backup/gitlab

# 2. 备份 GitLab 数据
sudo docker exec -t gitlab gitlab-backup create

# 3. 备份配置文件
sudo cp -r /srv/gitlab/config /backup/gitlab/config-$(date +%Y%m%d)

# 4. 创建完整备份脚本
sudo tee /usr/local/bin/gitlab-backup.sh > /dev/null << 'EOF'
#!/bin/bash
BACKUP_DIR="/backup/gitlab"
DATE=$(date +%Y%m%d_%H%M%S)

# 创建备份目录
mkdir -p $BACKUP_DIR/$DATE

# GitLab 数据备份
docker exec -t gitlab gitlab-backup create

# 复制备份文件
docker cp gitlab:/var/opt/gitlab/backups $BACKUP_DIR/$DATE/
docker cp gitlab:/etc/gitlab $BACKUP_DIR/$DATE/config

# 压缩备份
cd $BACKUP_DIR
tar czf gitlab-backup-$DATE.tar.gz $DATE
rm -rf $DATE

echo "备份完成: gitlab-backup-$DATE.tar.gz"
EOF

# 5. 设置执行权限
sudo chmod +x /usr/local/bin/gitlab-backup.sh
```

#### 包安装方式备份

```bash
# 1. 创建 GitLab 备份
sudo gitlab-backup create

# 2. 备份配置文件
sudo tar czf /var/opt/gitlab/backups/gitlab-config-$(date +%Y%m%d).tar.gz /etc/gitlab

# 3. 列出备份文件
sudo ls -la /var/opt/gitlab/backups/
```

#### 设置定期备份

```bash
# 添加定时任务
sudo crontab -e

# 添加以下行（每天凌晨 2 点备份）
0 2 * * * /usr/local/bin/gitlab-backup.sh

# 或者包安装方式
0 2 * * * gitlab-backup create CRON=1
```

### 9.2 数据恢复

#### Docker 方式恢复

```bash
# 1. 停止 GitLab 服务
sudo docker stop gitlab

# 2. 恢复配置文件
sudo tar xzf /backup/gitlab/gitlab-config-20231201.tar.gz -C /

# 3. 启动 GitLab
sudo docker start gitlab

# 4. 等待服务启动，然后恢复数据
sudo docker exec -it gitlab gitlab-backup restore BACKUP=20231201_12345678

# 5. 重启服务
sudo docker restart gitlab
```

### 9.3 系统维护

#### 定期维护任务

```bash
# 1. 查看系统状态
sudo docker exec -it gitlab gitlab-ctl status

# 2. 查看日志
sudo docker logs gitlab | tail -100

# 3. 清理旧的备份文件（保留最近 30 天）
find /backup/gitlab -name "gitlab-backup-*.tar.gz" -mtime +30 -delete

# 4. 检查磁盘空间
df -h /srv/gitlab
```

#### 更新 GitLab

```bash
# Docker 方式更新
# 1. 备份数据
/usr/local/bin/gitlab-backup.sh

# 2. 拉取最新镜像
sudo docker pull gitlab/gitlab-ce:latest

# 3. 停止旧容器
sudo docker stop gitlab
sudo docker rm gitlab

# 4. 启动新容器（使用相同配置）
sudo docker run --detach \
  --hostname gitlab.company.local \
  --publish 443:443 \
  --publish 80:80 \
  --publish 22:22 \
  --name gitlab \
  --restart always \
  --volume /srv/gitlab/config:/etc/gitlab \
  --volume /srv/gitlab/logs:/var/log/gitlab \
  --volume /srv/gitlab/data:/var/opt/gitlab \
  gitlab/gitlab-ce:latest
```

---

## 10. 常见问题解决

### 10.1 访问问题

#### 问题：无法访问 GitLab 页面

**症状**：浏览器显示"无法连接"或超时

**排查步骤**：

```bash
# 1. 检查容器是否运行
sudo docker ps | grep gitlab

# 2. 检查端口是否监听
sudo netstat -tlnp | grep -E ':80|:443'

# 3. 检查防火墙
sudo ufw status
# 如果启用了防火墙，需要开放端口
sudo ufw allow 80
sudo ufw allow 443

# 4. 检查 GitLab 日志
sudo docker logs gitlab | tail -50
```

**常见原因**：
- GitLab 还在启动中（等待 5-10 分钟）
- 防火墙阻断
- 端口被其他程序占用

#### 问题：DNS 解析失败

**症状**：`gitlab.company.local` 无法解析

**解决方案**：

```bash
# 方案 1：检查 hosts 文件
# Windows: C:\Windows\System32\drivers\etc\hosts
# Linux: /etc/hosts
# 确保包含：192.168.1.10 gitlab.company.local

# 方案 2：直接使用 IP 访问
http://192.168.1.10

# 方案 3：配置内网 DNS（需要网络管理员）
```

### 10.2 性能问题

#### 问题：GitLab 响应很慢

**排查步骤**：

```bash
# 1. 检查系统资源
free -h              # 内存使用情况
df -h                # 磁盘空间
top                  # CPU 使用情况

# 2. 检查 GitLab 进程
sudo docker exec -it gitlab gitlab-ctl status

# 3. 检查数据库状态
sudo docker exec -it gitlab gitlab-psql -c "SELECT version();"
```

**优化建议**：

```ruby
# 在 /srv/gitlab/config/gitlab.rb 中添加
# 减少 Worker 进程数量（适合小团队）
unicorn['worker_processes'] = 2
sidekiq['concurrency'] = 8

# 禁用不需要的功能
prometheus_monitoring['enable'] = false
alertmanager['enable'] = false
grafana['enable'] = false
```

### 10.3 权限问题

#### 问题：无法推送代码

**症状**：`git push` 时提示权限错误

**排查步骤**：

```bash
# 1. 检查用户权限
# 在 GitLab Web 界面：项目 → Members
# 确保用户至少有 Developer 权限

# 2. 检查分支保护
# 项目 → Settings → Repository → Protected branches
# 确保用户有推送权限

# 3. 检查认证
git config --list | grep user
# 确保用户名和邮箱正确
```

#### 问题：SSH 连接失败

**症状**：使用 SSH 方式克隆失败

**解决方案**：

```bash
# 1. 生成 SSH 密钥
ssh-keygen -t rsa -C "zhangsan@company.local"

# 2. 添加公钥到 GitLab
# 复制公钥内容
cat ~/.ssh/id_rsa.pub

# 3. 在 GitLab 中添加 SSH 密钥
# 用户头像 → Preferences → SSH Keys → Add key

# 4. 测试连接
ssh -T git@gitlab.company.local
```

### 10.4 升级问题

#### 问题：升级后无法启动

**症状**：升级 GitLab 后服务无法正常启动

**解决步骤**：

```bash
# 1. 查看启动日志
sudo docker logs gitlab

# 2. 检查配置文件
sudo docker exec -it gitlab gitlab-ctl reconfigure

# 3. 如果失败，回滚到备份版本
sudo docker stop gitlab
sudo docker rm gitlab

# 恢复之前的镜像版本
sudo docker run --detach \
  --hostname gitlab.company.local \
  --publish 443:443 \
  --publish 80:80 \
  --publish 22:22 \
  --name gitlab \
  --restart always \
  --volume /srv/gitlab/config:/etc/gitlab \
  --volume /srv/gitlab/logs:/var/log/gitlab \
  --volume /srv/gitlab/data:/var/opt/gitlab \
  gitlab/gitlab-ce:14.10.0  # 使用之前稳定的版本
```

### 10.5 存储空间问题

#### 问题：磁盘空间不足

**症状**：GitLab 报错，无法推送大文件

**解决方案**：

```bash
# 1. 检查磁盘使用情况
du -sh /srv/gitlab/*

# 2. 清理旧的备份文件
find /var/opt/gitlab/backups -name "*.tar" -mtime +7 -delete

# 3. 清理 Git 仓库（小心操作）
sudo docker exec -it gitlab gitlab-rake gitlab:cleanup:repos

# 4. 扩展磁盘空间（根据实际情况）
# 可能需要添加新硬盘或扩展现有分区
```

---

## 📋 部署检查清单

### 部署前检查

- [ ] 服务器硬件满足最低要求（4GB RAM, 50GB 存储）
- [ ] 操作系统已安装并更新（推荐 Ubuntu 22.04）
- [ ] 网络 IP 地址已规划（固定内网 IP）
- [ ] DNS 或 hosts 文件已配置
- [ ] 防火墙规则已设置
- [ ] Docker 已安装并测试

### 部署后检查

- [ ] GitLab Web 界面可以正常访问
- [ ] root 账户密码已修改
- [ ] 基础安全设置已配置
- [ ] 测试用户和项目已创建
- [ ] Git 推拉操作测试成功
- [ ] 备份脚本已设置并测试
- [ ] 监控和维护计划已制定

### 团队使用前检查

- [ ] 所有团队成员都能访问 GitLab
- [ ] 用户账户和权限已正确设置
- [ ] 项目已创建并配置保护规则
- [ ] Merge Request 流程已测试
- [ ] 团队成员已接受使用培训

---

## 🎯 总结

通过本教程，你已经学会了：

1. **理解 GitLab CE**：知道它是什么，为什么要用
2. **准备工作**：硬件、软件、网络规划
3. **部署 GitLab**：Docker 和包安装两种方式
4. **基础配置**：安全设置、用户管理、项目创建
5. **团队协作**：Git 工作流、Merge Request 流程
6. **运维管理**：备份、恢复、升级、故障排除

### 下一步建议

1. **小规模试用**：先用 2-3 人测试完整流程
2. **制定规范**：结合团队实际情况制定 Git 使用规范
3. **培训团队**：组织团队成员学习新的协作流程
4. **逐步推广**：确认稳定后再推广到整个团队

### 技术支持

- **GitLab 官方文档**：https://docs.gitlab.com/
- **社区论坛**：https://forum.gitlab.com/
- **问题追踪**：https://gitlab.com/gitlab-org/gitlab/-/issues

---
