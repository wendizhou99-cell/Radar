# AI Interface Compliance Guidelines

**Critical Issue**: AI agents frequently generate code that doesn't align with existing interfaces and data types, creating integration problems.

## 🚨 MANDATORY PRE-CODING VERIFICATION

### Rule 1: Interface Contract Enforcement
**BEFORE writing ANY code, AI MUST:**

1. **Read ALL existing interface files first**:
   ```bash
   # Required files to read BEFORE coding:
   include/common/types.h          # All data type definitions
   include/common/interfaces.h     # Base interface contracts
   include/common/error_codes.h    # Error handling patterns
   include/modules/[target_module].h # Specific module interface
   ```

2. **Verify data type compatibility**:
   - Use ONLY types defined in `types.h` (ComplexFloat, AlignedFloatVector, etc.)
   - NO custom data types unless explicitly requested
   - NO standard library types where project types exist (use AlignedFloatVector not std::vector<float>)

3. **Verify interface inheritance**:
   - ALL modules MUST inherit from `IModule` base interface
   - ALL processing classes MUST implement required virtual methods
   - NO standalone classes that bypass the interface hierarchy

### Rule 2: Function Signature Compliance
**Function signatures MUST match existing patterns:**

```cpp
// ✅ CORRECT - Matches project patterns
class MyProcessor : public IDataProcessor {
public:
    ErrorCode initialize(const ProcessorConfig& config) override;
    ErrorCode process(const RawDataPacket& input, ProcessedData& output) override;
    ErrorCode cleanup() override;

    // Use project types
    ErrorCode processRadarData(const AlignedComplexVector& input,
                              AlignedFloatVector& output);
};

// ❌ WRONG - Custom types and signatures
class MyProcessor {  // Missing inheritance
public:
    bool init(MyConfig config);  // Wrong return type, wrong parameter type
    std::vector<float> process(std::vector<std::complex<double>> data);  // Wrong types
    void doSomething(CustomDataType data);  // Undefined custom type
};
```

### Rule 3: Module Integration Verification
**Before implementing module interactions:**

1. **Check existing module interfaces**:
   ```cpp
   // Read these files to understand integration patterns:
   src/modules/data_receiver/    # How modules are structured
   src/modules/data_processor/   # Interface implementation patterns
   include/modules/             # Available interfaces for integration
   ```

2. **Verify callback patterns**:
   ```cpp
   // ✅ Use existing callback types from interfaces.h
   using ProcessingCompleteCallback = std::function<void(const ProcessingResult&)>;
   using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;

   // ❌ Don't create custom callback types
   using MyCustomCallback = std::function<void(bool, int)>;  // WRONG
   ```

## 🔧 INTEGRATION VERIFICATION CHECKLIST

**Before submitting any code, AI MUST verify:**

### Data Flow Compatibility
- [ ] Input data types match producer module output types
- [ ] Error codes use the project's layered error system (0x0000-SystemErrors, 0x1000-DataReceiverErrors, etc.)
- [ ] Callback functions match existing patterns in `interfaces.h`
- [ ] Memory management follows RAII patterns with project smart pointer usage

### Interface Inheritance Compliance
- [ ] Class inherits from appropriate base interface (`IModule`, `IDataProcessor`, etc.)
- [ ] All pure virtual methods are implemented
- [ ] Method signatures EXACTLY match base interface declarations
- [ ] Error handling follows project patterns (ErrorCode return values)

### Configuration Integration
- [ ] Configuration uses existing `ConfigManager` singleton
- [ ] Config structure matches patterns in `config.yaml`
- [ ] No hardcoded values - all configurable parameters use YAML config

### Logging Integration
- [ ] Uses project logging macros (`RADAR_INFO`, `RADAR_ERROR`, `RADAR_DEBUG`)
- [ ] No direct cout/printf - all output through logging system
- [ ] Log messages follow project format conventions

## 🎯 AI BEHAVIOR ENFORCEMENT

### Mandatory AI Response Pattern
When asked to implement a module/component, AI MUST respond with:

```markdown
## Implementation Plan Verification

Before I begin coding, I need to verify interface compliance:

### 1. Required File Analysis
I will first read these files to ensure compatibility:
- `include/common/types.h` - for data type definitions
- `include/common/interfaces.h` - for base interface contracts
- `include/modules/[relevant_module].h` - for specific module interface
- `configs/config.yaml` - for configuration patterns

### 2. Integration Points Check
- **Input/Output Types**: [List expected input/output types from existing interfaces]
- **Error Handling**: Will use ErrorCode return pattern with project error codes
- **Base Interface**: Will inherit from [SpecificInterface] base class
- **Configuration**: Will integrate with ConfigManager for [ConfigSection]

### 3. Module Interaction Verification
- **Producer Module**: [Module that provides input data]
- **Consumer Module**: [Module that receives output data]
- **Data Type Flow**: [Specific data types passed between modules]

**Proceeding with implementation only after confirmation of above compliance.**
```

### Code Generation Rules

1. **NEVER create new data types** without explicit user request
2. **ALWAYS use existing project types** from `types.h`
3. **ALWAYS inherit from project interfaces**
4. **ALWAYS use project error handling patterns**
5. **ALWAYS integrate with existing configuration system**

### Verification Commands
AI should suggest these verification commands after code generation:

```bash
# Verify compilation with existing interfaces
cmake --build build --target [module_name]

# Check interface compliance
grep -r "class.*:" include/modules/[module_name].h
grep -r "ErrorCode" src/modules/[module_name]/

# Verify data type usage
grep -r "ComplexFloat\|AlignedFloatVector\|ProcessedData" src/modules/[module_name]/
```

## 🚫 FORBIDDEN PATTERNS

### Absolutely Prohibited:
- Creating custom data types when project types exist
- Implementing standalone classes without interface inheritance
- Using std::vector<float> instead of AlignedFloatVector
- Using bool/int return codes instead of ErrorCode
- Direct file I/O instead of ConfigManager
- cout/printf instead of RADAR_LOG macros
- Custom error handling instead of project error codes

### Red Flag Indicators:
```cpp
// These patterns indicate non-compliance:
class SomeClass {           // Missing interface inheritance
bool doSomething() {        // Wrong return type (should be ErrorCode)
std::vector<float> data;    // Wrong data type (should be AlignedFloatVector)
std::cout << "message";     // Wrong logging (should be RADAR_INFO)
MyCustomType data;          // Undefined custom type
```

## ⚡ EMERGENCY COMPLIANCE CHECK

If user reports integration issues, AI MUST immediately:

1. **Review interface compliance** of the generated code
2. **Check data type consistency** with project types
3. **Verify inheritance hierarchy** matches project patterns
4. **Propose specific fixes** for each compliance violation

**Remember: Code that doesn't integrate is worse than no code at all.**
