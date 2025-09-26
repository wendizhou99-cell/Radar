# 指令：如何为 Radar 项目贡献代码

欢迎！本文件旨在指导您（AI助手）如何为我们的雷达数据处理系统（代号：Radar）编写高质量、符合架构规范的代码和文档。请在生成任何内容前，务必遵循以下核心原则和规范。

## 1. 核心架构原则 (The Four Pillars)

这是项目的基石，所有设计和实现都必须严格遵守。

1.  **事件驱动架构 (Event-Driven Architecture)**
    - **严禁模块间直接调用**。所有跨模块通信**必须**通过中央的 `EventBus` 进行。
    - 模块作为事件的发布者或订阅者，实现完全解耦。
    - 事件是标准的、自包含的数据结构，通常继承自 `BaseEvent`。

2.  **依赖注入 (Dependency Injection)**
    - **严禁使用全局单例或静态实例**。
    - 所有依赖（如 `IConfigManager`, `IEventBus`, `ILogger`）**必须**通过构造函数注入 `std::shared_ptr`。
    - 这种模式保证了模块的可测试性和依赖关系的清晰性。

3.  **职责分离 (Separation of Concerns)**
    - **决策者 vs 执行者**: `TaskScheduler` 是系统生命周期和恢复策略的唯一“决策者”。其他所有模块都是“执行者”，只负责完成自己的任务并上报状态/错误。
    - **单一职责**: 每个模块/服务都有明确且唯一的职责。例如：
        - `LoggingService`: 只负责日志记录。
        - `MonitoringModule`: 只负责监控和告警事件发布。
        - `ConfigManager`: 是配置的唯一真相来源（Single Source of Truth）。

4.  **全链路可观测性 (End-to-End Observability)**
    - **`TraceID` 是第一公民**。所有数据流的起点（如 `DataReceiver`）必须生成 `TraceID`。
    - `TraceID` **必须**在整个处理链中（包括数据对象、事件、日志）无损传递。
    - 日志**必须**是结构化的，并自动包含 `TraceID`、模块名、线程ID等上下文信息。

## 2. C++ 编码规范与风格

- **语言标准**: C++17。
- **错误处理**: **严禁使用异常**。所有可能失败的函数都必须返回 `ErrorCode`。
- **内存管理**:
    - 优先使用智能指针 (`std::unique_ptr`, `std::shared_ptr`)。
    - 在性能关键路径（如 `DataReceiver` 到 `SignalProcessor` 的数据流），**必须**采用零拷贝策略，通过指针或句柄传递页锁定内存（Pinned Memory）。
- **并发模型**:
    - 优先使用无锁数据结构（如SPSC/MPSC队列）进行跨线程通信。
    - 避免在热点路径上使用互斥锁。
    - 业务线程的指标更新应使用 `thread_local` 缓存，并采用“主动推送”模式聚合。
- **命名约定**:
    - **接口**: `I` 前缀，如 `IModule`, `ILogger`。
    - **类/结构体**: `PascalCase`，如 `DataProcessor`, `SystemMetrics`。
    - **函数/方法**: `camelCase`，如 `initialize`, `processData`。
    - **成员变量**: `snake_case_` 后缀，如 `event_bus_`。
- **日志**:
    - **必须**使用项目提供的日志宏 (`RADAR_INFO`, `RADAR_DEBUG` 等)。
    - 日志消息**必须**使用 `fmt` 风格的格式化字符串，如 `RADAR_INFO(logger_, "Packet received: size={}", size);`。

## 3. 核心组件与设计模式

在生成代码时，请遵循以下组件的设计模式和职责。

- **`IModule`**: 所有主要功能单元的基础接口。由 `TaskScheduler` 管理其生命周期 (`initialize`, `start`, `stop` 等)。
- **`TaskScheduler`**: 系统的“大脑”。负责模块注册、生命周期管理、依赖关系解析和故障恢复决策。
- **`EventBus`**: 系统的“神经网络”。所有模块通过它进行异步、解耦的通信。
- **`ConfigManager`**: 系统的“配置中心”。
    - 负责加载分层配置 (`base.yaml`, `modules/*.yaml`, `environments/*.yaml`)。
    - 模块**不应**直接读取文件，而应在启动时从 `ConfigManager` 获取配置快照。
    - 配置热更新**必须**通过订阅 `CONFIG_CHANGED` 事件来实现。
- **`ExecutionEngine`**: 模块内部的主循环驱动器。用于主动模块（如 `MonitoringModule`）或复杂处理流水线（如 `DataProcessor`）。
- **策略模式 (Strategy Pattern)**:
    - 核心算法（如关联、滤波、聚类、渲染）**必须**抽象为策略接口（如 `IAssociator`, `IRendererStrategy`）。
    - 具体实现作为可插拔的策略类，通过 `AlgorithmFactory` 根据配置动态创建。这使得算法的替换和扩展无需修改核心逻辑。
- **`DataContext`**: 在处理流水线中传递数据的上下文对象。它**不拥有**资源，只作为数据指针和元数据（包括`TraceID`）的载体。

## 4. 文档与图表规范

- **文档结构**: 新的模块设计文档应遵循现有文档的结构（职责、架构、约束等）。
- **图表**: **必须**使用 Mermaid 语法。
    - **架构图**: 使用 `graph` 或 `flowchart`，清晰展示组件、层次和依赖关系。
    - **时序图**: 使用 `sequenceDiagram`，清晰展示事件流和组件交互。
    - **状态机**: 使用 `stateDiagram-v2`，清晰展示生命周期和状态转换。
- **注释**: 对关键代码、复杂逻辑和性能优化点提供清晰的Doxygen风格注释。

## 5. 示例：当被要求“添加一个新功能”时

**错误的做法**:
```cpp
// 在 DataProcessor.cpp 中
void DataProcessor::someNewFunction() {
    // 直接调用 SignalProcessor 的方法
    g_signalProcessor->doSomething();
    // 直接修改配置
    g_config.setValue("some_key", 123);
}
```

**正确的做法**:
1.  **定义事件**: 在 `events.h` 中定义一个新的事件，如 `NewFeatureRequestedEvent`。
2.  **发布事件**: `DataProcessor` 在满足条件时，通过注入的 `event_bus_` 发布此事件。
    ```cpp
    // 在 DataProcessor.cpp 中
    void DataProcessor::triggerNewFeature() {
        auto event = std::make_shared<NewFeatureRequestedEvent>();
        event->trace_id = this->current_trace_id_;
        event_bus_->publish(event);
    }
    ```
3.  **订阅事件**: `SignalProcessor` 在其构造函数或初始化方法中，订阅该事件。
    ```cpp
    // 在 SignalProcessor.cpp 中
    SignalProcessor::SignalProcessor(...) {
        event_bus_->subscribe<NewFeatureRequestedEvent>([this](const auto& e) {
            this->onNewFeatureRequested(e);
        });
    }
    ```
4.  **处理事件**: 在 `SignalProcessor` 中实现事件处理函数 `onNewFeatureRequested`。
5.  **配置变更**: 如果需要修改配置，应发布 `ConfigChangeRequestEvent`，由 `ConfigManager` 处理。
