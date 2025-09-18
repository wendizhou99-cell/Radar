# GitHub Copilot Personal Instructions Setup Guide

## 📋 Overview

GitHub Copilot's personal custom instructions feature allows developers to set personal preferences that apply to all projects. These instructions work in both chat panel and immersive mode, helping Copilot better understand your coding style and project requirements.

This guide is specifically tailored for the radar data processing system project, providing best practices and recommended configurations.

## 🎯 How to Add Personal Custom Instructions

### Step 1: Access GitHub Copilot

On any GitHub page, you can open GitHub Copilot in two ways:

1. **Immersive Mode**:
   - Click the 🤖 icon in the top-right corner of the page
   - Opens Copilot conversation assistant in full-page immersive mode

2. **Chat Panel**:
   - Click the dropdown next to the 🤖 icon
   - Select "Assistive" to open the Copilot conversation assistant panel

### Step 2: Open Personal Instructions Settings

1. Find the settings menu (⚙️) in the top-right corner of the chat panel or immersive page
2. Click the dropdown menu
3. Select "Personal instructions"

### Step 3: Write Personal Instructions

Add your natural language instructions in the text box. You can use any preferred format:

- **Paragraph Format**: Write all instructions as coherent paragraphs
- **List Format**: Put each instruction on a separate line
- **Block Format**: Separate different categories of instructions with blank lines

### Step 4: Use Templates (Optional)

Click the template button (📝) to see available common instruction templates:

- **Communication**: Communication style settings
- **Code Style**: Code style preferences  
- **Documentation**: Documentation format requirements
- **Testing**: Testing methodology preferences

When you select a template, placeholders (like `{format}`) will appear in the text box that you can replace with your personal preferences.

### Step 5: Save Settings

Click the "Save" button. Your personal instructions will take effect immediately and remain active until modified or deleted.

## 🚀 Recommended Personal Instructions for Radar Project

### Basic Configuration Template

```
I'm developing a GPU-accelerated phased array radar data processing system using C++17, CUDA, and CMake.

Code Style Requirements:
- Follow project naming conventions: radar:: namespace, member variables with underscore suffix, private methods in camelCase
- Use project-specific data types: ComplexFloat, AlignedFloatVector, AlignedComplexVector
- All modules must inherit from IModule base interface
- Error handling uses ErrorCode return values, no exceptions
- Use RADAR_INFO, RADAR_ERROR macros for structured logging

Performance Requirements:
- Code must support real-time processing with <10ms latency requirements
- Prioritize GPU memory management and CPU-GPU data transfer optimization
- Use memory-aligned data structures for SIMD operations
- Avoid dynamic memory allocation in performance-critical paths

Documentation Requirements:
- Use Doxygen format comments with @brief, @param, @return
- Document performance characteristics and thread safety requirements in comments
- Add @warning tags for initialization order dependencies and resource management

Please check include/common/interfaces.h, types.h, and error_codes.h for interface compatibility before generating code.
```

### Advanced Configuration Template

```
Project Context:
I'm developing a modular radar processing system in the radar_mvp/ directory. The system uses layered architecture: application layer, business layer, data layer, and infrastructure layer.

Technology Stack Preferences:
- Build System: CMake 3.16+, supporting MSVC/GCC/Clang
- Dependency Management: Managing third_party libraries via git submodules
- Testing Framework: GoogleTest for unit and integration testing
- Logging System: spdlog with RADAR_* macros
- Configuration Management: YAML format, ConfigManager singleton pattern

Architecture Patterns:
- All modules implement IModule interface
- Use Strategy pattern for CPU/GPU computation switching
- Apply Factory pattern for creating processing strategies
- Event-driven inter-module communication

Quality Standards:
- Compile with -Wall -Wextra -Werror flags
- Code coverage target >80%
- Support cross-platform (Windows/Linux)
- Follow C++ Core Guidelines

Development Workflow:
1. First define interfaces in include/modules/
2. Implement concrete classes in src/modules/[module_name]/
3. Immediately perform error checking and compilation verification
4. Integrate into RadarApplication main application class
5. Add corresponding unit tests

Always prioritize interface compatibility and real-time performance requirements.
```

### Simplified Configuration Template

```
I'm developing a C++ radar processing system. Please:
- Use project's radar:: namespace and ErrorCode error handling
- Prioritize GPU performance and memory alignment
- Follow IModule interface inheritance pattern
- Use RADAR_* logging macros and Doxygen comment format
- Check existing interface compatibility before generating code
```

## 🛠️ Professional Advice

### Best Practices for Writing Instructions

1. **Specificity Over Generality**
   ```
   ❌ Write good code
   ✅ Use radar:: namespace, inherit IModule interface, return ErrorCode status codes
   ```

2. **Include Project Context**
   ```
   ❌ Optimize performance
   ✅ Optimize GPU memory transfer for real-time radar processing, target latency <10ms
   ```

3. **Specify Technical Constraints**
   ```
   ❌ Use modern C++
   ✅ Use C++17 features, support CUDA, avoid exception handling, prioritize smart pointers
   ```

### Common Configuration Mistakes

1. **Overly Broad Instructions**
   - Avoid: "Write high-quality code"
   - Recommend: "Follow project coding standards, use specific data types and error handling patterns"

2. **Missing Project-Specific Information**
   - Avoid: Just saying "C++ project"
   - Recommend: Clearly state "GPU-accelerated radar processing system using CUDA with real-time constraints"

3. **Ignoring Performance Requirements**
   - Avoid: Not mentioning performance
   - Recommend: Specify latency requirements, memory alignment, GPU optimization needs

### Instruction Update Strategy

- **Regular Review**: Check monthly if instructions match project evolution
- **Version Control**: Record history of important instruction changes
- **Team Sync**: Discuss common instruction patterns with team members
- **Experimental Testing**: Test new instructions' impact on code generation quality

## 🔧 Validating Personal Instructions Effectiveness

### Testing Methods

1. **Simple Code Generation Test**
   ```
   Ask Copilot: "Create a new data processor module interface"
   Verify generated code:
   - Uses radar:: namespace
   - Inherits from IModule base class
   - Uses project-specific data types
   - Includes appropriate Doxygen comments
   ```

2. **Error Handling Test**
   ```
   Ask: "Add error checking logic"
   Verify:
   - Uses ErrorCode return values
   - Avoids throwing exceptions
   - Uses project-defined error codes
   ```

3. **Performance Optimization Test**
   ```
   Ask: "Optimize this loop for performance"
   Verify:
   - Considers GPU acceleration
   - Uses memory-aligned data structures
   - Avoids dynamic memory allocation
   ```

### Effectiveness Evaluation Metrics

- **Code Consistency**: Percentage of generated code conforming to project conventions
- **Compilation Success Rate**: First-time compilation pass rate for generated code
- **Performance Awareness**: Whether real-time processing requirements are automatically considered
- **Interface Compatibility**: Whether existing interfaces and types are used correctly

## 📚 Related Resources

### Project Documentation References

- [Coding Standards and Style Guide](../03_技术规范/编码规范与代码风格指南.md)
- [Architecture and Modular Development Standards](../03_技术规范/架构与模块化开发规范.md)
- [GitHub Copilot Project Instructions](.github/copilot-instructions.md)

### GitHub Official Documentation

- [GitHub Copilot Chat Documentation](https://docs.github.com/en/copilot/github-copilot-chat)
- [Copilot Personal Instructions](https://docs.github.com/en/copilot/customizing-copilot/adding-custom-instructions-for-github-copilot)

## 🚨 Important Notes

### Security Considerations

- **Avoid Sensitive Information**: Don't include passwords, keys, or internal system information in personal instructions
- **Protect Proprietary Technology**: Avoid detailed descriptions of core algorithms or trade secrets
- **Review Generated Content**: Always check AI-generated code for security compliance

### Best Practice Reminders

- **Progressive Optimization**: Start with simple instructions, gradually add more details
- **Team Coordination**: Discuss common instruction patterns with team to avoid conflicts
- **Documentation Sync**: Keep personal instructions consistent with project documentation
- **Regular Updates**: Adjust personal instructions as the project evolves

---

**Last Updated**: September 18, 2025  
**Applicable Version**: GitHub Copilot Latest Version  
**Maintainer**: Radar Project Development Team

By properly configuring personal instructions, you can help GitHub Copilot better understand the specific requirements of the radar project, improving the accuracy and project compliance of code generation. For any questions or suggestions, please refer to project documentation or contact the development team.