# Project Instructions for GitHub Copilot

## 1. General Project Context
- **Language Standard:** C++20 or higher. Use modern C++ features (concepts, ranges, `std::format`, `std::span`) where appropriate instead of older patterns.
- **Compiler:** Microsoft Visual Studio 2022 (MSVC).
- **Platform:** Windows API (Win32). No cross-platform wrappers (like Qt or wxWidgets) unless specified.

## 2. Windows API Coding Style
- **Global Scope Prefix:** Always prefix WinAPI function calls with the global scope resolution operator `::`.
    - **Correct:** `::MessageBox(nullptr, ...);`
    - **Incorrect:** `MessageBox(nullptr, ...);`
    - *Reasoning:* This clearly distinguishes WinAPI functions from class methods or local wrappers.

- **Character Set & Function Names:** 
    - Use **Generic mappings** for WinAPI functions (e.g., `MessageBox`, `CreateWindow`, `GetWindowText`).
    - **DO NOT** use explicit `W` or `A` suffixes (like `MessageBoxW`).

## 3. Resource Files (.rc)
- The project supports multilingual resources.
- When editing or creating `.rc` files, be aware of language directives.
- Use standard resource definitions compatible with VS2022 resource editor.
- String tables should be organized by language ID.

## 4. Code Formatting
- Use `nullptr` instead of `NULL` or `0` for pointers.
- Include headers neatly: `<windows.h>` first, then standard library, then project headers.
- Use `std::` namespace for standard library components.