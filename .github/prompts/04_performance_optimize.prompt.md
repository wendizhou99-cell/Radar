```markdown
# 04_performance_optimize.prompt.md
目的：针对 CUDA kernel 或关键数据流提出可落地的优化方案与 micro-benchmark。

注意：全局性能约束（如对齐/内存策略）见仓库级文档 .github/copilot-instructions.md。

---
Strong constraints（强约束 — 必须遵守）
- 明确给出性能目标（例如 latency < {LATENCY_MS} ms 或 throughput >= {THROUGHPUT}），模板必须包含这些数值占位符。
- 优化建议不得改变 ABI 或引入未批准的第三方库。
- 对每个建议提供可验证的 micro-benchmark（代码或具体命令），并说明测量工具（nsight、nvprof、或者 chrono 基准）。

Weak constraints（弱约束 — 推荐）
- 列出预期的资源影响（GPU 内存、CPU 利用、同步点）。
- 给出逐步回滚或 A/B 测试建议，以便在问题出现时恢复。
- 提供伪码或关键代码片段，而不是完整实现（除非请求完整实现）。

模板示例
"下面是待优化的 kernel/函数（我会粘上代码）。目标：{LATENCY_MS} ms，内存对齐 64 字节，减少 CPU↔GPU 拷贝次数。请：
1) 列出 3 个主要瓶颈；
2) 为每个瓶颈给出 1 个可实现方案（含关键代码片段或伪码）；
3) 提供 micro-benchmark 示例代码和测量命令。"
```
