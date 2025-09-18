---
applyTo: "**/*.cpp,src/**/*.cpp,**/*.cu,src/**/*.cu"
---

# Implementation File Rules for Radar MVP System

## Purpose
Implement interfaces with performance, correctness, and testability in mind. Keep changes incremental and verifiable.

## Core Rules

**Interface Implementation**:
- Implement interfaces declared under `include/`. Adhere strictly to declared contracts
- Return `ErrorCode` for recoverable failures; avoid throwing exceptions in performance-critical paths
- Use smart pointers (std::unique_ptr/std::shared_ptr) for ownership. Factory functions return smart pointers
- Use RADAR logging macros (RADAR_INFO, RADAR_ERROR, RADAR_DEBUG) for structured logging

**Code Organization**:
- Keep functions small and focused; extract helpers into anonymous/impl namespaces
- Place GPU kernels in `.cu` files; provide thin, well-documented C++ wrappers in `.cpp`
- Annotate thread-safety and memory alignment requirements for buffer handling
- Update `src/CMakeLists.txt` when adding new files; integrate into `radar_modules` static lib

**Performance & Safety**:
- Avoid dynamic allocations in hot loops; use preallocated aligned buffers
- Document performance characteristics and resource requirements
- Use project-specific aligned types (AlignedComplexVector) for GPU data

## Quality & Delivery

**Style & Verification**:
- Follow Google C++ Style Guide (use project `.clang-format`)
- When uncertain about implementation details, follow Intent Clarification rule: ask 1–3 questions or list up to 5 missing items
- For large changes, split into small steps and run automated checks:
  1. CMake configure + build (affected targets)
  2. clang-format check and basic static analysis
  3. Unit tests related to changed module
- Report PASS/FAIL results before proceeding to next step

**Testing & Integration**:
- Add unit tests in `tests/unit_tests/` for non-trivial logic
- Temporary assumptions must be marked with `// TODO` plus rationale and resolution steps

## Configuration & Lifecycle

```cpp
// Example: Module implementation pattern
ErrorCode DataProcessor::initialize(const ProcessorConfig& config) {
    config_ = config;
    // Initialize GPU resources, buffers, etc.
    return SystemErrors::SUCCESS;
}
```

## 中文补充
实现文件应明确体现配置读取点（ConfigManager）、模块生命周期（initialize/run/stop）、以及错误码映射。GPU 相关实现需在文件头注明最低 CUDA 版本和 compute capability 要求。
