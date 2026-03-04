# Overview

## Project Goals

URUS is a programming language designed to be **simple, safe, and easy to learn** — while being powerful enough to build real applications. URUS transpiles to C, producing fast native binaries without requiring a heavy runtime.

## Problems Solved

| Problem | URUS Solution |
|---------|---------------|
| C is too low-level and prone to memory bugs | Automatic ref-counting, strict type safety |
| Python/JS are too slow for some use cases | Compiles to native C binary |
| Rust's learning curve is too steep | Simple syntax, focused feature set |
| New languages often lack documentation | Complete documentation from v1.0.0 |

## Target Users

- **Students** who want to learn programming language and compiler concepts
- **Developers** who need a simple language with native performance
- **Educators** who teach compiler design
- **Hobbyists** who enjoy exploring new languages

## High-Level Explanation

```
Source Code (.urus)
       │
       ▼
  ┌─────────┐
  │  Lexer   │  Tokenize source code
  └────┬─────┘
       ▼
  ┌─────────┐
  │  Parser  │  Build Abstract Syntax Tree (AST)
  └────┬─────┘
       ▼
  ┌─────────┐
  │  Sema    │  Type checking & semantic analysis
  └────┬─────┘
       ▼
  ┌─────────┐
  │ Codegen  │  Generate C code
  └────┬─────┘
       ▼
  ┌─────────┐
  │   GCC   │  Compile C to binary
  └────┬─────┘
       ▼
  Native Binary (.exe / ELF)
```

URUS takes a **transpiler** approach: source code is first translated to C, then GCC compiles the C into a binary. This provides:
- Native performance without a VM
- Portability to all platforms supported by GCC
- Easier debugging (you can inspect the generated C)

## Tech Stack

| Component | Technology |
|-----------|------------|
| Compiler | C11 (self-contained, no dependencies) |
| Runtime | Header-only C library (`urus_runtime.h`) |
| Memory Management | Reference counting |
| Backend | GCC (C compiler) |
| Build System | Make (Linux/macOS), build.bat (Windows) |
| Target Output | Standard C11 code |
