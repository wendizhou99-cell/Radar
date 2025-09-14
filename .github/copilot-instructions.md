# GitHub Copilot Instructions for Radar MVP System

This is a **GPU-accelerated phased array radar data processing system** built in C++ with modular architecture and real-time performance requirements.

## 🚨 CRITICAL: Interface Compliance Requirements

**MANDATORY**: Before any code generation, read and follow `.github/ai-interface-compliance.md` for strict interface compliance rules.

**Key Violations to Avoid**:
- Creating custom data types instead of using existing project types from `types.h`
- Implementing classes without proper interface inheritance from `interfaces.h`
- Using standard library types where project-specific types exist (e.g., std::vector<float> vs AlignedFloatVector)
- Module integration without checking existing interface contracts

**Pre-Coding Verification Required**:
1. Read `include/common/types.h`, `include/common/interfaces.h`, `include/common/error_codes.h`
2. Verify target module interface in `include/modules/[module_name].h`
3. Check integration patterns in existing module implementations

## Core Architecture

**Main Structure**: `radar_mvp/` contains the MVP implementation with layered architecture:
- `include/common/` - Core interfaces and data types shared across modules (interfaces.h, types.h, error_codes.h)
- `include/modules/` - Module-specific interfaces (data_receiver, data_processor, task_scheduler, display_controller)
- `src/modules/` - Module implementations with concrete classes (base_receiver.cpp, hardware_receiver.cpp, etc.)
- `src/application/` - System orchestration layer (RadarApplication class with full lifecycle management)
- `third_party/` - External dependencies managed as git submodules (spdlog, yaml-cpp, googletest)

**Key Pattern**: All modules inherit from `IModule` base interface in `common/interfaces.h`. Standardized error handling uses layered error codes (0x0000-SystemErrors, 0x1000-DataReceiverErrors, etc.) with `ErrorCode` return values instead of exceptions.

## Build System

**CMake Configuration**: Multi-level CMakeLists.txt structure:
- Root: Global compiler settings, C++17 standard, MSVC/GCC/Clang support with specific optimization flags
- `src/CMakeLists.txt`: Creates static libraries (radar_common, radar_modules, radar_application) with PUBLIC/PRIVATE dependency linking
- Third-party libraries linked via git submodules (not package managers)

**Critical Dependencies**: Initialize submodules FIRST with `.\download_dependencies.ps1` before any build attempts.

**Build Commands**:
```powershell
# Windows PowerShell (REQUIRED sequence)
.\download_dependencies.ps1  # Initialize git submodules - MUST run first
cmake -B build -S .
cmake --build build --config Release
```

## Development Workflow

**Module Development Pattern** (based on `docs_private/03_开发指南/模块开发流程.md`):
1. Define interface in `include/modules/[module_name].h`
2. Implement in `src/modules/[module_name]/` directory (note: modules have subdirectories)
3. Link to radar_common library for shared utilities
4. Use structured logging via spdlog with RADAR_* macros (RADAR_INFO, RADAR_ERROR)

**File Creation Order** (critical for dependencies from `docs_private/03_开发指南/文件创建顺序.md`):
1. `include/common/types.h` - Basic type definitions (ComplexFloat, Timestamp, AlignedVectors)
2. `include/common/error_codes.h` - Layered error codes (SystemErrors::SUCCESS, DataReceiverErrors::*, etc.)
3. `include/common/interfaces.h` - Abstract base classes (IModule, callback function types)
4. Module interfaces in `include/modules/`
5. Module implementations in `src/modules/[module_name]/`
6. Application layer (`src/application/radar_application.cpp`) orchestrates all modules

## Project-Specific Conventions

**Logging**: Use structured logging with `RADAR_INFO`, `RADAR_ERROR` macros. Logger initialization required in main.cpp before any module usage.

**Configuration**: Centralized YAML config in `configs/config.yaml`, managed by ConfigManager singleton. Configuration sections: system, data_receiver, data_processor, task_scheduler, display_controller.

**Error Handling**: Layered error code system (0x0000-SystemErrors, 0x1000-DataReceiverErrors, 0x2000-DataProcessorErrors, etc.). Methods return `ErrorCode` rather than exceptions for performance-critical paths.

**Naming Conventions** (from actual codebase):
- Namespace pattern: `radar::[layer]` (e.g., `radar::common`, `radar::modules`, `radar::application`)
- Member variables: underscore suffix (`member_`)
- Private methods: camelCase (`methodName`)
- Public interfaces: camelCase (`publicMethod`)
- Constants: ALL_CAPS_UNDERSCORE (`MAX_BUFFER_SIZE`)
- Atomic variables: descriptive suffix (`running_`, `shouldStop_`)
- Mutexes: descriptive + Mutex (`bufferMutex_`, `statsMutex_`)

**Data Types**: GPU-optimized types like `ComplexFloat`, `AlignedFloatVector`, `AlignedComplexVector` for I/Q signal processing.

## AI Interaction Guidelines

**Document-First Approach**: Always request specific documentation from `docs_private/` before starting development. Use `docs_private/00_快速开始/文档智能索引.md` to identify required documents.

**Required Context Documents** for different tasks:
- Module development: `docs_private/03_开发指南/模块开发流程.md` + `docs_private/02_编码规范/代码风格指南.md`
- Interface design: `docs_private/02_编码规范/设计模式应用.md`
- Performance work: `docs_private/09_最佳实践/性能优化指南.md`

**Template Usage**: Leverage `docs_private/06_自动化模板/模块模板/` for consistent code structure.

## Core AI Workflow Principles

**MANDATORY Interface Compliance Check**: Before any code generation, AI must read and verify compliance with existing interfaces. See `.github/ai-interface-compliance.md` for detailed enforcement rules.

**CRITICAL: Immediate Error Validation Rule** ⚠️
**Every code file MUST be validated for errors immediately after creation before proceeding to the next file.**

**Mandatory Error Checking Workflow**:
1. **Complete ONE file at a time** - Never create multiple files in sequence without validation
2. **Immediate Error Check** - Use `get_errors` tool right after each file creation/modification
3. **Fix ALL errors** - Resolve every compilation error before proceeding
4. **Verify Fix** - Re-run error check to confirm resolution
5. **Only then proceed** - Move to next file only after current file is error-free

**Why This Rule Is Critical**:
- **Prevents Error Cascade**: Early errors compound into complex debugging sessions
- **Maintains Code Quality**: Ensures each component is solid before building upon it
- **Reduces Debugging Time**: Fix issues when context is fresh, not after building entire modules
- **Interface Compliance**: Catches interface mismatches immediately, not after entire modules are built

**Error Check Triggers** (MUST check after):
- Creating any new `.h` or `.cpp` file
- Modifying existing interface or type definitions
- Adding new class declarations or function signatures
- Changing inheritance hierarchies or template parameters
- Completing any logical code block or component

**Error Resolution Priority**:
1. **Compilation Errors** - Fix immediately, these block all other work
2. **Interface Mismatches** - Critical for module integration
3. **Type Conflicts** - Often indicate design issues requiring architectural fixes
4. **Template/Generic Issues** - Can cascade into multiple files

**Intent Clarification Protocol** (from `docs_private/01_AI交互规范/核心交互原则.md`):
- Use specific technical terms, avoid ambiguous descriptions
- Provide concrete performance requirements and constraints
- Include sufficient context about project stage (MVP/optimization/refactoring)
- Minimize round-trip communications: aim for 3 conversation rounds per module development

**Requirements Analysis Thinking** (from `docs_private/07_AI工作流/需求分析流程.md`):
- Extract key information: identify task type (develop/optimize/integrate), technical objects, quality requirements
- Perform intent reasoning: analyze business goals, usage scenarios, integration needs, constraint analysis
- Evaluate system impact: assess data layer, service layer, frontend, and deployment implications

**Progressive Code Generation** (from `docs_private/07_AI工作流/代码生成流程.md`):
1. **Stage 0**: MANDATORY - Interface compliance verification (read existing interfaces and types)
2. **Stage 1**: Interfaces and data structures (compilable with existing types)
3. **Stage 2**: Core implementation skeleton (runnable with proper inheritance)
4. **Stage 3**: Complete functionality (feature complete with integration verified)
5. **Stage 4**: Integration and optimization (production ready with full compliance)

## Design Pattern Applications

**Strategy Pattern Usage** (from `docs_private/02_编码规范/设计模式应用.md`):
- Use for CPU/GPU processing switches, algorithm selection, performance optimization strategies
- Implement `IProcessingStrategy` interface with concrete strategies (`GPUProcessingStrategy`, `CPUProcessingStrategy`)
- Include performance characteristics: latency, throughput, power consumption, memory requirements

**Module Interface Design**:
- All modules inherit from `IModule` base interface
- Use factory patterns for strategy creation (`AuthStrategyFactory`, `ProcessorFactory`)
- Apply facade pattern for unified service interfaces (`AuthenticationService`, `DataProcessingService`)

## Code Style Standards

**C++17 Modern Features** (from `docs_private/02_编码规范/代码风格指南.md`):
- Use `auto` for obvious or complex types, avoid for basic types unless explicit
- Prefer smart pointers for resource management: `std::unique_ptr` for ownership, `std::shared_ptr` for sharing
- Factory methods return smart pointers, interface parameters use raw pointers or references

**File Organization Strategy**:
```
src/modules/[module_name]/
├── include/
│   ├── interfaces/     # Public interface definitions
│   ├── types/         # Data type definitions
│   └── [module_name].h # Main module header
├── src/
│   ├── strategies/    # Strategy implementations
│   ├── factories/     # Factory class implementations
│   └── [service].cpp  # Main service implementation
├── tests/
│   ├── unit/          # Unit tests
│   └── integration/   # Integration tests
└── configs/
    └── [config].yaml  # Configuration examples
```

**Layered Architecture Implementation**:
- **Presentation Layer**: API controllers, data validation, response formatting
- **Business Layer**: Service classes, strategy implementations, workflow orchestration
- **Data Layer**: Repository interfaces, entity models, data transformation
- **Infrastructure Layer**: Configuration management, logging, exception handling

## Critical Integration Points

**GPU Processing**: Code must support both CPU and GPU execution paths. GPU-specific code likely uses CUDA (check CMakeLists.txt for CUDA flags).

**Real-time Requirements**: Performance-critical modules must meet latency requirements defined in system specs. Use async processing patterns where specified.

**Thread Management**: Task scheduler coordinates module lifecycle. Modules should be thread-safe and support graceful shutdown.

**Data Flow**: Follows receiver → processor → visualizer pipeline with task scheduler orchestrating execution.

## Testing Strategy

**Test Structure**: Separate unit tests (`tests/unit_tests/`) and integration tests (`tests/integration_tests/`).

**Test Dependencies**: GoogleTest framework available via third_party. Link radar module libraries for testing.

**Test Patterns**: Create test data files for modules (see `test_data_receiver.cpp` example with `createTestDataFile()` helper).

When implementing new features, always consider the real-time processing requirements and modular architecture constraints specific to radar systems.

## Performance Optimization Guidelines

**Performance Analysis Framework** (from `docs_private/09_最佳实践/性能优化指南.md`):
- **Computing Performance**: Throughput, latency, CPU utilization, concurrency
- **Memory Performance**: Usage patterns, allocation efficiency, cache hit rates, bandwidth utilization
- **I/O Performance**: Disk/network I/O rates, wait times, queue depths
- **GPU Performance**: Utilization, memory usage, data transfer rates, kernel execution times

**AI-Generated Code Performance Patterns**:
- AI code tends to be overly conservative with excessive safety checks
- Look for unnecessary validations in tight loops (performance bottleneck)
- Batch operations instead of per-item processing for GPU workloads
- Profile memory allocation patterns - AI may not optimize for reuse

**Optimization Priorities for Radar Systems**:
1. **GPU Memory Management**: Minimize CPU-GPU data transfers, use pinned memory
2. **Real-time Constraints**: Meet latency requirements (typically <100ms for radar processing)
3. **Batch Processing**: Process multiple radar packets together for better GPU utilization
4. **Memory Alignment**: Use aligned data structures for SIMD and GPU operations

## Dependency Management Strategy

**Layered Dependency Model** (from `docs_private/03_开发指南/依赖管理策略.md`):
```
Application Layer → Business Layer → Service Layer → Infrastructure Layer → Third-party Libraries
```

**Dependency Principles**:
- **Unidirectional Dependencies**: Upper layers depend on lower layers, never reverse
- **Interface Segregation**: Modules depend only on interfaces they need
- **Dependency Inversion**: High-level modules depend on abstractions, not implementations
- **Stable Dependencies**: Depend on stable modules, avoid frequently changing dependencies

**Third-party Library Categories**:
- **Core Runtime**: CUDA Runtime, spdlog, yaml-cpp (essential for system operation)
- **Development Tools**: GoogleTest, CMake tools (build and test only)
- **Optional Features**: OpenGL, Boost (feature-specific dependencies)

## Debugging Strategies for AI-Generated Code

**AI Code Debugging Challenges** (from `docs_private/09_最佳实践/调试技巧.md`):
- Logic paths may be more complex than expected due to AI's safety-first approach
- Multiple error handling branches can obscure the actual problem
- Performance issues often hidden behind "safe" implementations

**Debugging Approach**:
1. **Understand AI Logic**: Read AI-generated comments to understand the intended design pattern
2. **Identify Bottlenecks**: Look for excessive validation loops and redundant safety checks
3. **Trace Data Flow**: Follow data transformations through the pipeline using logging
4. **Use Profiling Tools**: GPU profilers (nsight) for CUDA code, CPU profilers for host code

**Common AI Code Anti-patterns to Watch For**:
- Excessive input validation in performance-critical paths
- Overly generic solutions for specific radar processing needs
- Unnecessary memory allocations in real-time processing loops
- Complex error handling that obscures core algorithm logic

## Anti-patterns to Avoid

**Over-dependency Anti-patterns** (from `docs_private/09_最佳实践/反模式警示.md`):
- Don't blindly accept all AI suggestions without reviewing for project constraints
- Avoid AI-generated "universal" solutions that add unnecessary complexity
- Maintain control over architectural decisions, use AI for implementation guidance

**Over-engineering Anti-patterns**:
- Resist AI tendency to create overly abstract designs for simple needs
- Avoid unnecessary design patterns for straightforward radar data processing
- Keep solutions proportional to actual requirements, not theoretical extensibility

**Documentation Standards** (from `docs_private/02_编码规范/注释和文档规范.md`):
- Use Doxygen format for all public interfaces: `@brief`, `@param`, `@return`, `@note`
- Include performance characteristics in comments: latency expectations, memory usage
- Document thread safety requirements explicitly for radar real-time constraints
- Add `@warning` for initialization order dependencies and resource management

## Development Environment Setup

**System Requirements** (from `docs_private/04_技术栈配置/硬件环境要求.md`):
- **Windows**: Windows 10 20H2+ or Windows 11, x64 architecture, 16GB+ RAM (32GB recommended)
- **Linux**: Ubuntu 22.04 LTS preferred, kernel 5.15+, x86_64 architecture
- **GPU**: NVIDIA GPU with CUDA 12.2+ support, 8GB+ VRAM for optimal performance
- **Storage**: 50GB+ available space, SSD recommended for build performance

**Critical Environment Setup** (from `docs_private/04_技术栈配置/开发环境配置.md`):
```powershell
# Windows PowerShell setup sequence
choco install -y git cmake ninja visualstudio2022buildtools cuda
code --install-extension ms-vscode.cpptools ms-vscode.cmake-tools nvidia.nsight-vscode-edition
```

**Performance Targets for Real-time Processing**:
- **Data Processing Latency**: Target <5ms, Critical <10ms, Maximum <20ms
- **GPU Computation**: Target <2ms, Critical <5ms for CUDA kernel execution
- **Throughput**: Target 1GB/s data ingestion, 10k samples/ms signal processing
- **Resource Utilization**: CPU 60-80% optimal, GPU 70-90% optimal, Memory <70%

## Pre-Development Checklist

**CRITICAL Interface Compliance** (MUST be completed FIRST):
- [ ] **Read Interface Contracts**: Review `include/common/interfaces.h` for base interface definitions
- [ ] **Verify Data Types**: Check `include/common/types.h` for all available project data types
- [ ] **Review Error Patterns**: Understand `include/common/error_codes.h` for error handling conventions
- [ ] **Check Module Interface**: Read target module interface in `include/modules/[module_name].h`
- [ ] **Study Integration Examples**: Review existing module implementations for patterns

**Essential Preparation** (from `docs_private/00_快速开始/检查清单.md`):
- [ ] **Compiler Verification**: GCC 9+ or MSVC 2019+ confirmed
- [ ] **CUDA Environment**: Run `nvcc --version`, compile CUDA samples
- [ ] **CMake Configuration**: Test build system with CMake 3.16+
- [ ] **Dependencies Ready**: Third-party libraries (OpenCV, Eigen, FFTW) verified
- [ ] **Hardware Resources**: GPU memory, compute capability, CPU cores confirmed
- [ ] **Code Templates**: Module templates from `docs_private/06_自动化模板/` prepared

**Documentation Review Requirements**:
- [ ] **Architecture Documentation**: Review latest system architecture from `docs/01_项目设计/`
- [ ] **Interface Specifications**: Check `include/interfaces/` for current API definitions
- [ ] **Coding Standards**: Familiar with `docs/03_技术规范/代码编写规范.md`
- [ ] **Radar System Conventions**: Understand data formats, processing flows, performance requirements

## Architecture Integration Analysis

**Coupling Analysis Framework** (from `docs_private/07_AI工作流/架构耦合分析.md`):
When developing new modules, AI must immediately analyze integration challenges:

1. **Data Source Analysis**: Where does data come from? Database, API, direct hardware interface?
2. **Interface Dependencies**: Which existing services need to be called? What are the interface contracts?
3. **Performance Impact**: Will this new module affect existing real-time processing pipelines?
4. **Data Structure Compatibility**: Are there type mismatches, naming convention conflicts, or missing fields?

**Proactive Integration Strategy**:
- **Adapter Pattern**: Create conversion layers for incompatible data structures (zero intrusion approach)
- **Unified Models**: Modify existing structures when feasible for long-term consistency
- **Interface Evolution**: Plan for backward compatibility when extending existing APIs
- **Performance Profiling**: Measure integration impact on existing real-time constraints

**Required Integration Information Request Template**:
> "To ensure proper integration with existing radar systems, please provide:
> 1. **Data Structure Definitions**: Relevant C++ headers, database schemas, or API models
> 2. **Existing Interface Contracts**: Service APIs, callback definitions, or communication protocols
> 3. **System Architecture Context**: Module interaction diagrams or dependency mappings"
