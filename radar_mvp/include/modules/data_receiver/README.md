# 数据接收模块模块化重构说明

## 概述

数据接收模块已经重构为模块化架构，以减少耦合并提高代码的可维护性。新的结构将原来的单一头文件拆分为四个专门的模块。

## 新的模块化结构

### 1. 基础接口模块 (`data_receiver_base.h`)
- **职责**: 定义数据接收器的基础抽象接口
- **内容**:
  - `DataReceiver` 基类
  - 基础类型定义和回调函数
  - 纯虚函数接口定义
  - RAII 资源管理

### 2. 统计信息模块 (`data_receiver_statistics.h`)
- **职责**: 性能监控和统计信息管理
- **内容**:
  - `ReceptionStatistics` 结构
  - `PerformanceMonitor` 类
  - `StatisticsManager` 类
  - 线程安全的统计收集

### 3. 具体实现模块 (`data_receiver_implementations.h`)
- **职责**: 各种具体的接收器实现类
- **内容**:
  - `UDPDataReceiver` - UDP网络接收器
  - `FileDataReceiver` - 文件数据接收器
  - `HardwareDataReceiver` - 硬件数据接收器
  - `SimulationDataReceiver` - 模拟数据接收器

### 4. 工厂模式模块 (`data_receiver_factory.h`)
- **职责**: 接收器创建和管理
- **内容**:
  - `DataReceiverFactory` 命名空间
  - 工厂方法和类型枚举
  - `ReceiverManager` 管理器类
  - 配置验证和默认配置

## 向后兼容性

为了保持向后兼容，主入口文件 `data_receiver.h` 仍然存在，它：
- 包含所有模块化头文件
- 提供类型别名到主命名空间
- 保持原有的API不变

## 使用方式

### 旧的使用方式（仍然支持）
```cpp
#include "modules/data_receiver.h"
using namespace radar;

auto receiver = DataReceiverFactory::createUDPReceiver(config);
```

### 新的模块化使用方式
```cpp
// 只需要基础接口
#include "modules/data_receiver/data_receiver_base.h"

// 需要统计功能
#include "modules/data_receiver/data_receiver_statistics.h"

// 需要具体实现
#include "modules/data_receiver/data_receiver_implementations.h"

// 需要工厂创建
#include "modules/data_receiver/data_receiver_factory.h"

using namespace radar::modules;
```

## 模块依赖关系

```
data_receiver_base.h (基础)
    ↑
data_receiver_statistics.h (统计)
    ↑
data_receiver_implementations.h (实现)
    ↑
data_receiver_factory.h (工厂)
    ↑
data_receiver.h (统一入口)
```

## 文件映射

| 原文件位置                                          | 新文件位置                                                      |
| --------------------------------------------------- | --------------------------------------------------------------- |
| `include/modules/data_receiver.h`                   | `include/modules/data_receiver.h` (入口)                        |
| -                                                   | `include/modules/data_receiver/data_receiver_base.h`            |
| -                                                   | `include/modules/data_receiver/data_receiver_statistics.h`      |
| -                                                   | `include/modules/data_receiver/data_receiver_implementations.h` |
| -                                                   | `include/modules/data_receiver/data_receiver_factory.h`         |
| `src/modules/data_receiver.cpp`                     | 拆分到各个对应的实现文件                                        |
| `src/modules/data_receiver/base_receiver.cpp`       | 保持不变                                                        |
| `src/modules/data_receiver/hardware_receiver.cpp`   | 保持不变                                                        |
| `src/modules/data_receiver/simulation_receiver.cpp` | 保持不变                                                        |
| `src/modules/data_receiver/receiver_factory.cpp`    | 保持不变                                                        |

## 优点

1. **降低耦合**: 每个模块专注于特定职责
2. **提高可维护性**: 更清晰的模块边界
3. **加速编译**: 按需包含所需模块
4. **便于测试**: 独立的模块更容易单元测试
5. **向后兼容**: 保持现有代码不受影响
6. **扩展性**: 更容易添加新的接收器类型

## 注意事项

1. 所有现有代码无需修改即可正常编译
2. 新开发建议使用模块化的包含方式
3. 编译系统自动处理依赖关系
4. 命名空间从 `radar` 改为 `radar::modules`，但通过别名保持兼容
