# Changelog

## v1.0.0 (2026-03-02)

### New Features
- **Enums / Tagged Unions**: `enum Shape { Circle(r: float); Rect(w: float, h: float); Point; }`
- **Pattern Matching**: `match` statement with variant bindings
- **Modules / Imports**: `import "module.urus";` with circular import detection
- **Error Handling**: `Result<T, E>` type with `Ok(val)` / `Err(msg)`, `is_ok()`, `is_err()`, `unwrap()`, `unwrap_err()`
- **String Interpolation**: `f"Hello {name}, age {age}"` desugars to to_str + concat
- **For-each Loops**: `for item in array { ... }` — iterate over array elements
- **Conversion Functions**: `to_int()` and `to_float()` now fully implemented

### Bug Fixes
- Fixed Makefile missing sema.c and codegen.c
- Fixed `ast_type_str` static buffer clobber (round-robin buffers)
- Fixed `urus_str_replace` unsigned underflow with signed diff
- Fixed array codegen removing GCC statement expressions (standard C11)
- Fixed array element types (now supports `[float]`, `[str]`, `[bool]`, `[MyStruct]`)
- Fixed array index assignment generating invalid C lvalue
- Fixed temp file path dead ternary
- Added bounds checking on array access
- Break/continue now validated to be inside loops

### Improvements
- Reference counting: `retain`/`release` functions for str, array
- Larger emit buffer (4096 from 2048)
- `to_str` now retains the string (proper refcounting)
- Empty function params emit `void` in C for correctness
- Version string in compiler output

## v0.1 (2025-03-01)

### Initial Release
- Primitive types: int, float, bool, str, void
- Variables with `let` / `let mut`, mandatory type annotation
- Functions with typed parameters and return types
- Control flow: if/else, while, for (range-based), break, continue
- Operators: arithmetic, comparison, logical, assignment
- Structs (declaration, literal creation, field access)
- Arrays (literal, indexing, len, push)
- String concatenation with `+`
- Comments (single-line `//`, multi-line `/* */`)
- 30+ built-in functions (string ops, math, file I/O, assert)
- Transpiles to C via GCC
