<!--
Sync Impact Report:
Version: 1.0.0 (Initial Constitution)
Creation Date: 2025-11-18
Modified Principles: N/A (Initial creation)
Added Sections: All sections (Core Principles, Coding Standards, Architecture Constraints, Development Workflow, Governance)
Removed Sections: N/A
Templates Alignment:
  ✅ plan-template.md - Aligned with C++17, CEF framework, Windows platform requirements
  ✅ spec-template.md - Aligned with testable requirements and user scenarios approach
  ✅ tasks-template.md - Aligned with phased implementation and testing discipline
Follow-up TODOs: None
-->

# CefView Constitution

## Core Principles

### I. C++17 Standard Compliance

**Description**: All code MUST strictly adhere to C++17 standard without using C++2a features or non-standard compiler extensions.

**Rationale**: Ensures maximum portability across different compilers and platforms while leveraging modern C++ capabilities. C++17 provides sufficient features (structured bindings, if constexpr, std::optional, std::variant) for robust application development.

**Non-negotiable Rules**:
- MUST use C++17 features only (no C++20/C++2a)
- MUST NOT use compiler-specific extensions (MSVC, GCC, Clang)
- MUST compile with standard conformance mode enabled
- MUST prefer standard library over platform-specific APIs where possible

### II. CEF Framework Integration

**Description**: All browser-related functionality MUST be implemented using the Chromium Embedded Framework (CEF) APIs and patterns, following CEF's multi-process architecture.

**Rationale**: CEF provides a production-ready, secure browser engine with clear separation between browser process and renderer process. Proper CEF integration ensures stability, security, and maintainability.

**Non-negotiable Rules**:
- MUST use CEF APIs for all web content rendering
- MUST implement proper CefApp, CefClient, and CefHandler patterns
- MUST respect CEF's multi-process architecture (browser, renderer, GPU processes)
- MUST handle inter-process communication (IPC) through CEF message routing
- MUST NOT bypass CEF's security model or sandboxing (when enabled)
- MUST properly manage CefRefPtr reference counting

### III. Windows Native UI

**Description**: All UI components MUST use Windows native APIs (Win32/User32) directly, avoiding third-party UI frameworks.

**Rationale**: Direct Win32 integration provides maximum control, minimal dependencies, and optimal performance. Native windows integrate seamlessly with Windows DPI scaling, theming, and accessibility features.

**Non-negotiable Rules**:
- MUST create windows using CreateWindowEx and Windows message loop
- MUST handle WM_* messages directly for window management
- MUST use HWND as the primary window handle type
- MUST support high DPI scaling through native Windows DPI APIs
- MUST NOT introduce Qt, wxWidgets, or other UI framework dependencies
- MUST implement proper window procedure callbacks (WndProc pattern)

### IV. Object-Oriented Design

**Description**: Code MUST be highly structured following object-oriented principles with clear separation of concerns, single responsibility, and proper encapsulation.

**Rationale**: OOP provides maintainable, testable, and extensible code architecture. Proper class design enables code reuse and reduces coupling between components.

**Non-negotiable Rules**:
- MUST organize code into well-defined classes with single responsibility
- MUST use proper access modifiers (public, protected, private)
- MUST prefer composition over inheritance
- MUST define clear interfaces and contracts between components
- MUST use namespaces to organize code logically (`namespace cefview`)
- MUST avoid god classes and global state
- MUST implement proper RAII for resource management

### V. Naming Conventions Consistency

**Description**: All code MUST follow established naming conventions consistently across the entire codebase.

**Rationale**: Consistent naming improves code readability, reduces cognitive load, and enables predictable code navigation. The project uses a hybrid convention based on observed patterns.

**Non-negotiable Rules**:

**Files**:
- MUST use PascalCase for headers and source files (e.g., `FileUtil.h`, `CefWebView.cpp`)
- MUST use `.h` for headers, `.cpp` for application code, `.cc` for utility/library code
- MUST use descriptive names reflecting the primary class/functionality

**Classes & Types**:
- MUST use PascalCase for class names (e.g., `CefManager`, `MainWindow`)
- MUST prefix CEF-related classes with `Cef` (e.g., `CefWebView`, `CefViewClient`)
- MUST use PascalCase for struct and enum class names

**Functions & Methods**:
- MUST use camelCase for function and method names (e.g., `loadUrl`, `setRect`)
- MUST use descriptive verb-noun combinations
- MUST use `get`/`set` prefixes for accessors/mutators

**Variables**:
- MUST use camelCase for local variables and function parameters
- MUST prefix member variables with underscore `_` (e.g., `_hwnd`, `_client`)
- MUST prefix global variables with `g_` (e.g., `g_instance`)
- MUST use `k` prefix for constants (e.g., `kPathSep`, `kBufferSize`)

**Namespaces**:
- MUST use lowercase for namespaces (e.g., `namespace cefview`, `namespace cefview::util`)
- MUST use nested namespaces with `::` separator (C++17 style)

**Macros**:
- MUST use ALL_CAPS with underscores for preprocessor macros
- MUST minimize macro usage, prefer `constexpr` and inline functions

### VI. Header Guards & File Organization

**Description**: All header files MUST use proper include guards and follow a consistent organization pattern.

**Rationale**: Prevents multiple inclusion issues and provides predictable file structure for developers.

**Non-negotiable Rules**:
- MUST use `#ifndef/#define/#endif` guards with UPPERCASE_FILENAME_H format
- MUST include `#pragma once` as additional protection
- MUST include copyright/documentation header with file, brief, version, author, date
- MUST organize includes in order: related header, C system, C++ standard, CEF, other libraries, project headers
- MUST use forward declarations to minimize header dependencies where possible
- MUST keep headers self-contained (can compile independently)

### VII. Code Quality & Safety

**Description**: Code MUST prioritize safety, correctness, and clarity over cleverness or brevity.

**Rationale**: Embedded browser applications require high reliability and security. Clear, safe code reduces bugs and security vulnerabilities.

**Non-negotiable Rules**:
- MUST use smart pointers (std::unique_ptr, std::shared_ptr) over raw pointers for ownership
- MUST use nullptr instead of NULL or 0
- MUST use range-based for loops where applicable
- MUST mark virtual destructors as virtual or override
- MUST use = delete for prohibited copy/move operations
- MUST validate all user inputs and external data
- MUST handle all error conditions explicitly
- MUST use const correctness throughout
- MUST initialize all variables at declaration
- MUST NOT ignore compiler warnings (treat warnings as errors)

## Coding Standards

### Google C++ Style Guide Compliance

**Baseline**: All code MUST follow Google C++ Style Guide unless explicitly overridden by project-specific conventions documented in this constitution.

**Key Requirements**:
- Line length: MUST NOT exceed 120 characters
- Indentation: MUST use 2 spaces (no tabs)
- Braces: MUST use K&R style (opening brace on same line)
- Comments: MUST use Doxygen-style documentation for public APIs
- Access specifiers: MUST order as public, protected, private with 1-space indent
- Member initialization: MUST use constructor initializer lists

### Platform Considerations

- MUST define UNICODE and use wide character APIs on Windows
- MUST handle Windows-specific paths with backslash separator
- MUST use Windows-native types (HWND, WPARAM, LPARAM) where appropriate
- MUST support Windows 10+ DPI awareness

### Modern C++ Features (C++17)

**MUST Use**:
- Smart pointers for memory management
- std::optional for optional values
- std::variant for type-safe unions
- Structured bindings for multi-value returns
- if constexpr for compile-time conditionals
- Inline variables
- std::string_view for non-owning string references
- Lambda expressions

**MUST NOT Use**:
- Manual new/delete (use smart pointers or containers)
- C-style casts (use static_cast, dynamic_cast, etc.)
- goto statements
- Multiple inheritance (except for interfaces)
- Mutable global state

## Architecture Constraints

### Project Structure

```
cef_view/
├── src/
│   ├── app/              # Application entry point and main window
│   ├── cefview/          # Core CEF integration library
│   │   ├── client/       # CEF client implementations
│   │   ├── handler/      # CEF event handlers
│   │   ├── manager/      # CEF lifecycle management
│   │   ├── view/         # Browser view components
│   │   └── utils/        # Utility functions
│   └── sub_process/      # CEF sub-process entry point
├── libcef/               # CEF binary distribution
├── CMakeLists.txt        # Build configuration
└── .specify/             # Project specifications and templates
```

### Component Responsibilities

**app/** - Application Layer
- MUST implement main window and application lifecycle
- MUST handle Windows-specific initialization
- MUST create and manage CefWebView instances
- MUST NOT contain CEF implementation details

**cefview/** - CEF Integration Layer
- MUST encapsulate all CEF-specific code
- MUST provide clean C++ interfaces to consumers
- MUST handle CEF initialization and shutdown
- MUST manage browser/renderer process coordination

**utils/** - Utility Layer
- MUST provide platform-agnostic utilities where possible
- MUST isolate platform-specific code in separate files (e.g., WinUtil.h)
- MUST NOT depend on CEF or application code

### Dependency Rules

- Application layer MAY depend on cefview and utils
- cefview layer MAY depend on utils and CEF
- utils layer MUST NOT depend on application or cefview
- MUST minimize circular dependencies through forward declarations
- MUST use dependency injection for testability

### Build System

- MUST use CMake (version 3.14+) as the build system
- MUST support Visual Studio 2022 (MSVC) compiler
- MUST support both Debug and Release configurations
- MUST support static linking (/MT) for dependencies
- MUST generate PDB files for all configurations
- MUST NOT require manual configuration beyond CEF path

## Development Workflow

### Version Control

- MUST use feature branches for all development (`###-feature-name`)
- MUST write descriptive commit messages
- MUST keep commits focused and atomic
- MUST NOT commit generated files or build artifacts
- MUST track untracked files through git status before commits

### Code Review Requirements

- All code changes MUST pass constitution compliance check
- MUST verify naming conventions match established patterns
- MUST verify proper CEF API usage and reference counting
- MUST verify Windows resource cleanup (HWND, HRGN, handles)
- MUST verify exception safety and RAII patterns
- MUST verify header guard consistency

### Testing Discipline (When Applicable)

While testing is not mandatory for all changes:
- If tests exist, they MUST pass before merge
- New components SHOULD include unit tests where practical
- CEF integration points SHOULD be manually tested
- Windows UI changes MUST be tested at multiple DPI scales
- MUST test on target Windows versions (10, 11)

### Documentation Requirements

- MUST document all public APIs with Doxygen comments
- MUST include file headers with @file, @brief, @version, @author, @date
- MUST document non-obvious implementation decisions
- MUST update README.md when build process changes
- MUST NOT create unnecessary documentation files unless explicitly requested

## Governance

### Constitution Authority

This constitution supersedes all other development practices and guidelines. When conflicts arise:
1. Constitution principles override team preferences
2. Explicit constitution rules override Google C++ Style Guide
3. Code clarity and safety override performance optimization
4. Established project patterns (observed in codebase) override external conventions

### Amendment Process

Amendments require:
1. Clear documentation of the proposed change
2. Justification with specific examples from codebase
3. Impact analysis on existing code
4. Version increment following semantic versioning:
   - MAJOR: Breaking changes to principles (e.g., switching to C++20)
   - MINOR: New principles or significant expansions
   - PATCH: Clarifications and typo fixes

### Compliance Verification

All code submissions MUST:
- Pass compiler warnings as errors (`/W4` on MSVC)
- Follow naming conventions (verified by code review)
- Use correct file extensions and header guards
- Maintain namespace organization
- Properly manage CEF reference counting
- Clean up Windows resources

### Complexity Justification

Any violation of these principles (e.g., introducing third-party UI framework, using C++20 features, avoiding RAII) MUST:
1. Be documented in implementation plan
2. Include justification for why simpler alternatives are insufficient
3. Provide migration path or containment strategy
4. Receive explicit approval before implementation

**Version**: 1.0.0 | **Ratified**: 2025-11-18 | **Last Amended**: 2025-11-18
