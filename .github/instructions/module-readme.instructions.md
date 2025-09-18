---
applyTo: "**/modules/**/README.md,src/modules/**/README.md,modules/**/README.md"
---

# Module README Rules for Radar MVP System

## Purpose
Each module README is a concise, actionable guide for maintainers and integrators: how to build, configure, use, test, and troubleshoot the module.

## Required Sections

**1. Overview**
- One-line responsibility statement and module boundaries
- Key dependencies and hardware requirements (especially GPU/CUDA)

**2. Public API**
- Referenced header files and key interfaces
- Minimal usage code snippet showing typical initialization and usage

**3. Configuration**
- YAML keys expected by ConfigManager with examples and defaults
- Performance tuning parameters

**4. Build & Test**
- CMake target names and build commands
- Unit test execution: `ctest -R ModuleNameUnitTests`
- Integration test scenarios

**5. Runtime Notes**
- Performance expectations (latency, throughput, memory usage)
- Threading model and GPU compute requirements
- Resource limits and monitoring points

**6. Troubleshooting**
- Common error scenarios and diagnostic commands
- Log message patterns to check
- Performance debugging tips

**7. Maintainers**
- Module owner and contact information

## Templates & Examples

**Configuration Example:**
```yaml
data_processor:
  enabled: true
  gpu_device: 0
  batch_size: 1024
  cuda_streams: 4
  buffer_size_mb: 256
```

**Build/Test Commands:**
```bash
# Build module
cmake --build build --target radar_modules

# Run unit tests
ctest -R DataProcessorUnitTests --verbose

# Run with specific config
./build/bin/radar_mvp --config configs/processor_test.yaml
```

## Quality Standards
- Follow Google C++ Style for code examples
- When uncertain about module specifications, ask clarifying questions rather than making assumptions
- Include performance benchmarks where applicable

## 中文补充
README 应聚焦"如何上手/如何验证/如何排查"，避免长篇背景说明。GPU 模块请列出 CUDA 版本、最低 compute capability、显存建议与常见调优点。
