# Changelog

See also: [CHANGELOG.md](../../CHANGELOG.md) in the project root.

## Format

The changelog follows the [Keep a Changelog](https://keepachangelog.com/) format:

```
## [version] - YYYY-MM-DD

### Added
- New features

### Changed
- Changes to existing features

### Fixed
- Bug fixes

### Removed
- Removed features
```

---

## [1.0.0] - 2026-03-02

### Added
- **Enums / Tagged Unions** — `enum Shape { Circle(r: float); Point; }`
- **Pattern Matching** — `match expr { Variant(x) => { ... } }`
- **String Interpolation** — `f"Hello {name}, count={count}"`
- **Modules / Imports** — `import "utils.urus";`
- **Error Handling** — `Result<T, E>`, `Ok(val)`, `Err(msg)`, `is_ok`, `is_err`, `unwrap`, `unwrap_err`
- **For-each Loops** — `for item in array { ... }`
- **Reference Counting** — `retain`/`release` for str, array
- **Array type support** — `[float]`, `[bool]`, `[str]` (previously only `[int]`)
- **Array index assignment** — `nums[i] = value;`
- **Built-in functions** — `to_int()`, `to_float()`, bounds checking
- **Break/continue validation** — error if outside a loop
- **File restructuring** — `compiler/src/` and `compiler/include/`
- **Test suite** — valid, invalid, and run tests with test runner
- **Documentation** — README, SPEC, CHANGELOG, CLAUDE.md, and documentation/ folder
- **Examples** — 9 example programs (hello, fibonacci, structs, arrays, enums, strings, result, files, modules)
- **License** — Apache 2.0

### Fixed
- `ast_type_str` static buffer clobber (round-robin 4 buffers)
- `urus_str_replace` unsigned underflow (use `ptrdiff_t`)
- GCC statement expressions `({...})` replaced with temp variable pattern (standard C11)
- Makefile missing `sema.c codegen.c` and `-lm`
- Dead ternary in temp file path
- Array codegen only supported `int` (now all types)
- Array index assignment generated invalid C lvalue

### Changed
- Compiler output is now standard C11 (no GCC extensions required)
- Enum/struct/array literals use temp variable pattern

---

## [0.1.0] - 2025

### Added
- Initial compiler: lexer, parser, sema, codegen
- Primitive types: `int`, `float`, `bool`, `str`, `void`
- Variables (immutable by default, `mut`)
- Functions
- Structs
- Arrays (`[int]` only)
- Control flow: `if/else`, `while`, `for..in` range
- Operators: arithmetic, comparison, logical, assignment
- Built-in: `print`, `len`, `push`, `to_str`
- String operations: `str_len`, `str_upper`, `str_lower`, `str_trim`, `str_contains`, `str_slice`, `str_replace`
- File I/O: `read_file`, `write_file`, `append_file`
- Comments: `//` and `/* */`
- Header-only runtime (`urus_runtime.h`)
