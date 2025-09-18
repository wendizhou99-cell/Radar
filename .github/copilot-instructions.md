# GitHub Copilot Instructions for Radar MVP System

This document provides essential guidelines for AI-assisted development on this project. Please adhere to these instructions to ensure code quality, consistency, and alignment with our architecture.

---

### 1. High-Level Project Goal (项目核心目标)

- **Primary Goal**: Develop a **GPU-accelerated phased array radar data processing system**.
- **Core Focus**: Real-time performance, modular architecture, and scalability.
- **Current Stage**: MVP (Minimum Viable Product) development. Focus on core functionality over extensive features.

---

### 2. Core Technologies & Stack (核心技术栈)

- **Programming Language**: C++17
- **Build System**: CMake
- **GPU Acceleration**: CUDA
- **Key Libraries**:
    - **Logging**: `spdlog` (Use `RADAR_INFO`, `RADAR_ERROR` macros)
    - **Configuration**: `yaml-cpp` (Load from `configs/config.yaml`)
    - **Testing**: `googletest`
- **(中文补充)**: *请在此处添加其他关键依赖库，例如 `Eigen`、`FFTW` 等。*

---

### 3. Architectural Principles (核心架构原则)

- **Modular Design**: The system is composed of independent modules (e.g., DataReceiver, DataProcessor). All modules must inherit from the `IModule` interface defined in `include/common/interfaces.h`.
- **Interface-Driven Development**: Always code against interfaces, not concrete implementations. Do not create custom data types if a suitable one exists in `include/common/types.h`.
- **Error Handling**: **No exceptions in performance-critical paths.** Use the `ErrorCode` return type as defined in `include/common/error_codes.h`. Each module has its own error code range (e.g., `DataReceiverErrors::*`).
- **Configuration Management**: All module configurations should be managed by a central `ConfigManager` and defined in `configs/config.yaml`.

---

### 4. Key Interfaces & Data Types (关键接口与数据类型)

This is a critical section. Before writing any code, you **MUST** familiarize yourself with these core components.

- **`IModule` (`include/common/interfaces.h`)**: The base interface for all modules. It defines the component lifecycle (`initialize`, `run`, `stop`).
- **`ErrorCode` (`include/common/error_codes.h`)**: The standard for returning function status. `SystemErrors::SUCCESS` indicates success.
- **Core Data Types (`include/common/types.h`)**:
    - `ComplexFloat`: For I/Q signal data.
    - `AlignedComplexVector`: For GPU-optimized data batches.
    - `Timestamp`: For data packet timing.
    - **(中文补充)**: *请优先使用这些项目特定的数据类型，而不是 `std::vector` 或原生类型，以确保性能和内存对齐。*

---

### 5. Development Workflow (模块开发流程)

Follow these steps to create a new module.

1.  **定义接口 (Define Interface)**: 在 `include/modules/` 目录下为你的新模块创建或修改头文件，定义其接口。
2.  **实现类 (Implement Class)**: 在 `src/modules/` 目录下创建 `.cpp` 文件，实现接口。确保你的实现类继承了正确的模块接口和 `IModule`。
3.  **添加构建规则 (Update CMake)**: 将新的源文件和头文件路径添加到 `src/CMakeLists.txt` 中，确保它被编译进 `radar_modules` 静态库。
4.  **集成到应用 (Integrate into Application)**: 在 `src/application/radar_application.cpp` 中，通过 `ConfigManager` 读取新模块的配置，并将其集成到系统生命周期中。
5.  **编写单元测试 (Write Unit Tests)**: 在 `tests/unit_tests/` 目录下为你的模块添加测试用例。

---

### 6. Coding Style & Conventions (编码风格与约定)

- **Naming Conventions**:
    - **Namespaces**: `radar::[layer]` (e.g., `radar::common`, `radar::modules`)
    - **Member Variables**: Use an underscore suffix (e.g., `member_`)
    - **Interfaces**: Abstract classes start with `I` (e.g., `IModule`)
    - **Constants**: `ALL_CAPS_UNDERSCORE`
- **Resource Management**:
    - Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for resource ownership.
    - Factory functions should return smart pointers.
- **Documentation**:
    - Use Doxygen-style comments (`@brief`, `@param`, `@return`) for all public headers.

- **Style Guide**:
    - Follow the Google C++ Style Guide for formatting, naming, includes and header organization. Use a clang-format configuration that matches Google style and apply it to all generated code. Maintain consistency with existing project formatting where it differs only for justified reasons.
    - (中文补充)：请在项目根目录提供或引用一个 `clang-format` 配置（建议基于 Google 风格），并在 CI 中运行格式检查。

---

### 7. Critical "Do's and Don'ts" (关键注意事项)

- **DO**: Use the project-defined types from `types.h`.
- **DO**: Return `ErrorCode` for functions that can fail.
- **DO**: Use the `RADAR_*` logging macros for structured logging.
- **DON'T**: Use `std::vector` for signal data; use `Aligned...Vector` instead.
- **DON'T**: Throw exceptions in the main data processing pipeline.
- **DON'T**: Create monolithic classes. Decompose functionality into smaller, single-responsibility components.

### 8. AI Collaboration Rules — Clarification & Iterative Delivery (AI 协作规则 — 澄清与迭代交付)

- Purpose:
    Provide clear rules for Copilot/AI behavior when generating code for this repository so suggestions are high-quality, non-invasive, and safe to integrate.

- Rules:
    - Intent clarification: When the agent is uncertain about requirements, design choices, or missing context, it MUST NOT make unilateral assumptions. Instead the agent should either:
        1. Ask 1–3 concise clarifying questions, or
        2. If user interaction is not possible, list up to 5 specific pieces of missing information it needs to proceed (for example: API contract, expected data shapes, error-code ranges, target CUDA compute capability, config YAML keys). Do not proceed until at least one of these items is answered.
        - (中文补充)：遇到不确定的地方，Copilot 要主动列出最多 5 项必要信息，例如接口签名、数据格式、性能目标、依赖版本、配置键名等。

    - Small-step delivery and automated checks: For large or risky changes, the agent must split the work into small, verifiable steps. After each change it should run or request the following automated checks and report results before continuing:
        1. Build (CMake configure + build) or at minimum a compilation check for affected files.
        2. Lint/format check (clang-format or project linter) and basic static analysis where available.
        3. Unit tests related to the changed module (if present) or a minimal smoke test.
        - The agent should present the results (PASS/FAIL and key errors) and propose the next small step. Only proceed after fixing failures or after user's explicit approval to continue.
        - (中文补充)：大改动请拆成小步，且每步完成后运行构建/格式/测试检查，把检查结果反馈给我；只有在检查通过或我批准后再继续下一步。

    - Error reporting and assumptions: When the agent must make temporary, well-scoped assumptions (for example placeholder types or TODOs), it must explicitly mark them with TODO comments and a short rationale, and include a short list of actions required to remove the assumptions.
        - (中文补充)：临时假设必须用 `// TODO` 标注，并说明原因与后续移除步骤。

- Compliance:
    - Copilot suggestions should strive to preserve existing public APIs and respect the project's interface-first rules. Any breaking API changes must be highlighted and require explicit approval.

(End of additions)
