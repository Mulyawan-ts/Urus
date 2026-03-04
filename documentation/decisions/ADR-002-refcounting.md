# ADR-002: Reference Counting for Memory Management

## Status
Accepted

## Context
We needed a safe memory management system without manual malloc/free in user code. Options:
1. Manual memory management (like C)
2. Garbage collector (mark-and-sweep)
3. Reference counting
4. Ownership system (like Rust)

## Decision
**Reference counting** for all heap-allocated values (str, array, struct).

## Rationale
- Deterministic destruction (predictable, no GC pauses)
- Simpler implementation compared to GC or ownership
- Well-suited for transpile-to-C (easy to generate retain/release calls)
- No runtime thread needed for GC

## Consequences
- **Positive:** Simple, predictable performance, no GC pauses
- **Negative:** Circular references cause memory leaks (no cycle detection), per-assignment overhead (retain/release)

## Future
Cycle detection can be added in a future version (weak references or backup tracing GC).
