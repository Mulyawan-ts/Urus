# ADR-003: Header-Only Runtime

## Status
Accepted

## Context
The runtime library (`urus_runtime.h`) contains implementations for string, array, print, file I/O, etc. We needed to decide whether the runtime should be:
1. Shared library (.so/.dll)
2. Static library (.a/.lib)
3. Header-only (everything inline in .h)

## Decision
**Header-only** — all runtime functions are defined in `urus_runtime.h` with `static inline` or inside an `#ifdef URUS_RUNTIME_IMPLEMENTATION` block.

## Rationale
- Zero dependency — generated C file only needs `#include "urus_runtime.h"` and GCC
- No separate link step needed
- Simple distribution — just copy a single file
- Well-suited for single-file generated output

## Consequences
- **Positive:** Very simple deployment, no linking issues, portable
- **Negative:** Slightly slower compile time (runtime compiled every time), slightly larger binary size (no sharing across programs)
