---
applyTo: "**/*.h,**/*.hpp,include/**/*.h,include/**/*.hpp"
---

# Header File Rules for Radar MVP System

## Purpose
Define stable, minimal public APIs. Headers must express contracts only — keep implementations out of public headers.

## Core Rules

**Interface Design**:
- Prefer interface-first design: public headers expose abstract interfaces (I-prefix for interfaces like `IModule`)
- Use `#pragma once` or traditional include guards
- Minimize includes: prefer forward declarations to reduce compile-time coupling
- Keep headers concise: aim < 300 lines; split large interfaces into smaller headers

**Documentation & Types**:
- Document all public symbols with Doxygen (`@brief`, `@param`, `@return`, `@note`)
- Use project core types from `include/common/types.h` (ComplexFloat, AlignedComplexVector, Timestamp)
- No `using namespace` in headers; avoid non-constexpr static state

**Error Handling**:
- Functions that may fail must use `ErrorCode` return values; avoid exceptions in hot APIs
- Reference error patterns from `include/common/error_codes.h`

**Performance Annotations**:
- If an API is GPU-aware, annotate alignment and thread-safety requirements
- Document performance characteristics: latency expectations, memory usage

## Examples

```cpp
// Good: Interface declaration
class IDataProcessor {
public:
    virtual ~IDataProcessor() = default;
    virtual ErrorCode initialize(const ProcessorConfig& config) = 0;
    virtual ErrorCode processData(const AlignedComplexVector& input,
                                  AlignedComplexVector& output) = 0;
};
```

## Quality & Style
- Follow Google C++ Style Guide for naming, formatting, and organization
- When uncertain about API design, ask 1-3 clarifying questions rather than making assumptions
- Apply `.clang-format` before committing

## Reference Files
- `include/common/interfaces.h` - Base interface patterns
- `include/common/error_codes.h` - Error handling conventions
- `include/common/types.h` - Project-specific data types

## 中文补充
头文件用于声明契约（接口、类型、常量）。实现相关细节放入 `.cpp` 或 private header。GPU 相关接口需要注明内存对齐要求和线程安全约束。
