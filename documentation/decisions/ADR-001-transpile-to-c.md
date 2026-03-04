# ADR-001: Transpile to C

## Status
Accepted

## Context
We needed to choose a target output for the URUS compiler. Options considered:
1. Directly to machine code (x86/ARM)
2. Transpile to C
3. LLVM IR
4. Bytecode + VM

## Decision
**Transpile to C.**

## Rationale
- C compilers (GCC/Clang) already handle optimization, register allocation, and platform support
- Debugging is easier — you can inspect generated C
- Automatic portability to all GCC-supported platforms
- Compiler complexity is much lower
- No large dependencies needed (LLVM = ~1GB)

## Consequences
- **Positive:** Fast development, portable, debuggable output
- **Negative:** Slower compile time (2 passes: URUS→C→binary), difficult to control low-level details
