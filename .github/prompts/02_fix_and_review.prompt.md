# 02_fix_and_review.prompt.md
目的：审查选中代码、列出问题并返回修复补丁（unified diff 格式）。

注意：请先参考仓库级约束（.github/copilot-instructions.md），该文件包含不重复的全局规则。

---
Strong constraints（强约束 — 必须遵守）
- 输出必须为 unified diff（patch），以便直接应用 git apply / git am。
- 不改变外部接口签名，除非给出向后兼容的迁移建议并列出迁移步骤。
- 修复中不能引入未在仓库中声明的新全局类型或依赖；如必须引入，提前说明理由并建议如何注册到仓库。
- 性能关键路径不得引入堆分配或阻塞调用（除非你说明并给出度量依据）。

Weak constraints（弱约束 — 推荐）
- 每个修复片段前写一行原因说明（1-2 句）。
- 若修复涉及算法/复杂逻辑，附上一到两个单元测试用例描述（不必实现）。
- 优先使用项目已有 helper 函数或 utilities。

使用方法（在 VSCode）
1. 在编辑器中选中目标函数/代码块并复制到 Chat。
2. 粘贴以下模板并附上选中代码。

模板（示例）
"我将粘贴一个待修复的代码片段。请：
1) 列出发现的 bug、边界条件、并按严重性排序（简短）。
2) 基于仓库约束输出修复补丁，格式为 unified diff。
3) 每个修复前写一句简要说明。
附上代码：<粘贴代码>"

---

## AI标准回答模板

当收到代码审查请求时，AI应按以下格式回复：

### 阶段1：问题分析

```markdown
## 代码审查结果

### 🔍 发现的问题（按严重性排序）

**🔴 严重问题（必须修复）**
- [ ] **[问题类型]**: [简短描述] - 影响：[具体影响]
- [ ] **[问题类型]**: [简短描述] - 影响：[具体影响]

**🟡 中等问题（建议修复）**
- [ ] **[问题类型]**: [简短描述] - 影响：[具体影响]

**🟢 轻微问题（可选修复）**
- [ ] **[问题类型]**: [简短描述] - 影响：[具体影响]

### 📋 检查清单验证
- [ ] 遵循Google C++风格指南
- [ ] 使用项目定义的ErrorCode返回类型
- [ ] 使用项目数据类型（types.h）
- [ ] 包含适当的RADAR_*日志宏
- [ ] 接口继承正确（IModule等）
- [ ] 没有引入未声明的依赖
- [ ] 性能关键路径无不必要分配
```

### 阶段2：修复方案

```markdown
## 修复方案

### 🛠️ 修复策略概述
[整体修复思路，1-2句话说明]

### 📊 性能影响评估
- **内存使用**: [无变化/减少X字节/增加Y字节，原因]
- **CPU开销**: [无变化/优化X%/增加微量开销，原因]
- **接口兼容性**: [完全兼容/需要迁移，迁移步骤]

### 🧪 建议测试用例
1. **正常情况测试**: [测试描述]
2. **边界条件测试**: [测试描述]
3. **异常情况测试**: [测试描述]
```

### 阶段3：修复补丁

```markdown
## 修复补丁（Unified Diff）

[修复原因简述] - 修复空指针检查缺失
```diff
--- a/src/modules/example/example.cpp
+++ b/src/modules/example/example.cpp
@@ -10,7 +10,10 @@

 ErrorCode ExampleClass::process(const InputData& input, OutputData& output) {
+    // 添加空指针检查确保输入有效性
+    if (!input.isValid()) {
+        return ExampleErrors::INVALID_INPUT;
+    }
+
     // 原有逻辑...
     return SystemErrors::SUCCESS;
 }
```

[修复原因简述] - 使用项目标准数据类型替换std::vector
```diff
--- a/include/modules/example/example.h
+++ b/include/modules/example/example.h
@@ -15,7 +15,7 @@
 class ExampleProcessor : public IModule {
 private:
-    std::vector<float> buffer_;
+    AlignedFloatVector buffer_;
 public:
```

### 🔧 应用补丁说明
```bash
# 保存补丁到文件
cat > fix.patch << 'EOF'
[上述diff内容]
EOF

# 应用补丁
git apply fix.patch

# 验证编译
cmake --build build --target affected_module

# 运行相关测试
ctest -R ExampleModule
```

### ⚠️ 注意事项
- 补丁应用后请运行完整的单元测试套件
- 如有接口变更，请更新相关文档
- 建议在应用前创建备份分支
```

### 阶段4：验证建议

```markdown
## 验证建议

### 📝 修复后检查清单
- [ ] 编译无警告通过：`cmake --build build`
- [ ] 单元测试通过：`ctest -R [ModuleName]`
- [ ] 格式检查通过：`clang-format --dry-run --Werror`
- [ ] 静态分析通过：`clang-tidy src/path/to/file.cpp`

### 🎯 性能验证（如适用）
- [ ] 基准测试：延迟未增加超过5%
- [ ] 内存检查：无新的内存泄漏
- [ ] GPU分析：无不必要的设备同步

### 📚 文档更新（如需要）
- [ ] API文档更新（如有接口变更）
- [ ] README.md更新（如有使用方式变更）
- [ ] 错误码文档更新（如有新错误类型）
```

---

## 变更记录

| 版本 | 日期       | 修改人 | 变更摘要                           |
| :--- | :--------- | :----- | :--------------------------------- |
| v1.0 | 2025-09-10 | Kelin  | 创建代码审查和修复prompt           |
| v1.1 | 2025-01-20 | AI     | 添加标准化AI回答模板和验证建议流程 |
