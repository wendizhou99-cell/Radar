# 02_fix_and_review.prompt.md
目的：审查选中代码、列出问题并返回修复补丁（unified diff 格式）。

注意：请先参考仓库级约束（.github/copilot-instructions.md），该文件包含不重复的全局规则。

---
Strong constraints（强约束 — 必须遵守）
- 输出必须是**完整的、可直接替换的代码块**，而不是补丁或diff。
- **必须**在每个代码块的顶部使用 `// filepath: path/to/your/file.cpp` 注释来清晰地标明文件路径。
- 不改变外部接口签名，除非给出向后兼容的迁移建议并列出迁移步骤。
// ...existing code...
模板（示例）
"我将粘贴一个待修复的代码片段。请：
1) 列出发现的 bug、边界条件、并按严重性排序（简短）。
2) 基于仓库约束输出**修复后的完整代码块**。
3) 每个修复的代码块前写一句简要说明。
附上代码：<粘贴代码>"

---

## AI标准回答模板
// ...existing code...
- **接口兼容性**: [完全兼容/需要迁移，迁移步骤]

### 🧪 建议测试用例
1. **正常情况测试**: [测试描述]
2. **边界条件测试**: [测试描述]
3. **异常情况测试**: [测试描述]
```

### 阶段3：修复代码

```markdown
## 修复代码

[修复原因简述] - 修复空指针检查缺失
```cpp
// filepath: src/modules/example/example.cpp
// ...existing code...
ErrorCode ExampleClass::process(const InputData& input, OutputData& output) {
    // 添加空指针检查确保输入有效性
    if (!input.isValid()) {
        RADAR_ERROR("Invalid input provided to ExampleClass::process");
        return ExampleErrors::INVALID_INPUT;
    }

    // 原有逻辑...
    return SystemErrors::SUCCESS;
}
// ...existing code...
```

[修复原因简述] - 使用项目标准数据类型替换std::vector
```cpp
// filepath: include/modules/example/example.h
// ...existing code...
#include "common/types.h" // 确保包含项目类型定义

class ExampleProcessor : public IModule {
private:
    AlignedFloatVector buffer_; // 使用项目定义的内存对齐向量
public:
// ...existing code...
```

### 🔧 应用代码说明
将上方代码块的内容，根据 `filepath` 提示，手动复制并替换到对应的文件中。

### ⚠️ 注意事项
- 代码替换后请运行完整的单元测试套件。
- 如有接口变更，请更新相关文档。
- 建议在修改前创建备份分支。
```

### 阶段4：验证建议
// ...existing code...
```

---

## 变更记录

| 版本 | 日期       | 修改人 | 变更摘要                           |
| :--- | :--------- | :----- | :--------------------------------- |
| v1.0 | 2025-09-10 | Kelin  | 创建代码审查和修复prompt           |
| v1.1 | 2025-01-20 | AI     | 添加标准化AI回答模板和验证建议流程 |
| v1.2 | 2025-09-22 | AI     | 将修复方式从补丁改为完整代码块     |
````

请确认这些修改是否符合您的需求，或者如果您有任何其他具体要求，请告诉我！
