好的，我们继续细化`03_配置接口设计.md`。

现在，我将为您详细阐述“4. 配置文件结构规范”。本章旨在建立一套清晰、可扩展的配置文件组织方式，确保`ConfigManager`能够高效地解析和分发配置，同时也让开发者和运维人员能够轻松地理解和修改系统参数。

-----

## 4\. 配置文件结构规范

  - **概要**: 为了让`ConfigManager`能够有效地解析和分发配置，所有配置文件都**必须**遵循统一的结构规范。本规范的核心是**分层**与**模块化**，它将复杂的系统配置分解为多个逻辑清晰、职责单一的文件，并通过`ConfigManager`的合并机制形成最终的运行时配置。

### 4.1. 分层结构

  - **概要**: 配置采用分层合并的策略，优先级从高到低为：**用户自定义配置 \> 环境特定配置 \> 模块默认配置 \> 系统基础配置**。`ConfigManager`负责在启动时按此顺序加载和**深度合并 (Deep Merge)** 这些文件，后加载的配置会覆盖先加载的同名键值。

**配置加载与覆盖顺序**:

1.  **`base.yaml` (系统基础配置)**

      * **职责**: 提供系统所有参数的默认值，确保即使在没有任何其他配置文件的情况下，系统也能以一种安全、基础的模式启动。
      * **优先级**: **最低**。

2.  **`modules/*.yaml` (模块配置)**

      * **职责**: 各模块在此目录下定义其详细、特定的配置参数。这有助于团队协作，不同模块的负责人可以独立维护自己的配置文件。
      * **优先级**: **高于** `base.yaml`。

3.  **`environments/*.yaml` (环境配置)**

      * **职责**: 定义不同部署环境（如`development`, `testing`, `production`）的特定覆盖参数。例如，在生产环境中，日志级别应更高，数据库地址也应不同。系统启动时会根据环境变量或命令行参数加载其中一个文件。
      * **优先级**: **高于** 模块配置。

4.  **`~/.radar_user.yaml` (用户配置 - 可选)**

      * **职责**: 允许开发者在本地覆盖特定参数以方便调试，而无需修改项目仓库中的任何配置文件。此文件不应被纳入版本控制。
      * **优先级**: **最高**。

**深度合并 (Deep Merge) 规则**:

  * 对于嵌套的对象（Maps），合并是递归的。
  * 对于简单值（字符串、数字、布尔值），后加载的值会完全替换先加载的值。
  * 对于数组（Lists），默认行为是**替换**。未来可根据需求在配置中添加元数据以支持**追加**模式。

### 4.2. 模块化命名空间

  - **概要**: 在YAML文件中，所有模块的配置**必须**位于以该模块`module_name`（与`ILifecycleManaged::getModuleName()`返回的字符串完全一致）命名的顶级键下。这使得`ConfigManager`可以轻松地为每个模块提取其专属的配置“切片”，也是`getInitialConfigFor(module_name)`方法能够工作的组织基础。

> **强制性契约**: 如果一个模块名为`DataReceiver`，那么它所有的配置项都必须位于`DataReceiver:`这个顶级YAML键之下。全局配置（如`system`, `logging`）是此规则的例外。

### 4.3. 示例配置文件

  - **概要**: 以下是一个简化的示例，展示了分层配置和模块化命名空间在实践中的应用。

#### `configs/base.yaml` (基础配置)

```yaml
# ----------------------------------
# 系统基础配置 (提供所有默认值)
# ----------------------------------
system:
  name: "RadarProcessingSystem"
  version: "v2.0.0"

# 全局服务配置
logging:
  level: "DEBUG"
  output: "console"

# 模块默认配置
DataReceiver:
  network:
    bind_address: "127.0.0.1"
    port: 9001
  buffer:
    pool_size_mb: 128

SignalProcessor:
  gpu_device_id: 0
  algorithm_pipeline:
    - name: "fft"
```

#### `configs/modules/signal_processor.yaml` (模块详细配置)

```yaml
# ----------------------------------
# SignalProcessor 模块的详细配置
# ----------------------------------
SignalProcessor:
  # 此处的 gpu_device_id 会覆盖 base.yaml 中的值
  gpu_device_id: 0

  # 详细定义算法流水线
  algorithm_pipeline:
    - name: "fft"
      enabled: true
      params:
        size: 4096
        window: "hanning"
    - name: "cfar"
      enabled: true
      params:
        type: "ca-cfar"
        threshold: 15.5 # 这是一个可热更新的参数
```

#### `configs/environments/production.yaml` (生产环境覆盖)

```yaml
# ----------------------------------
# 生产环境的覆盖配置
# ----------------------------------

# 覆盖全局日志级别
logging:
  level: "INFO"
  output: "file"
  file_path: "/var/log/radar.log"

# 覆盖 DataReceiver 的网络配置以监听所有接口
DataReceiver:
  network:
    bind_address: "0.0.0.0"
  buffer:
    # 增加生产环境的缓冲区大小
    pool_size_mb: 1024

# 覆盖 SignalProcessor 的一个可热更新参数
SignalProcessor:
  algorithm_pipeline:
    - name: "cfar"
      params:
        threshold: 18.0 # 生产环境使用更严格的阈值
```

**最终合并结果 (生产环境)**: 当系统以生产模式启动时，`ConfigManager`内部形成的最终配置视图将是以上文件的深度合并结果。例如，`SignalProcessor`的`threshold`将是`18.0`，而`DataReceiver`的`port`将是`9001`（因为它只在`base.yaml`中定义）。
