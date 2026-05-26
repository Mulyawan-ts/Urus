# Version History

This is a summary of each release. For the detailed changelog with every commit, bug fix, and contributor credit, see [CHANGELOG.md](../../CHANGELOG.md) in the project root.

> **Note on the version reset:** The project was renumbered from `0.3.0` back
> to `0.1.0` in May 2026 to signal that the language is in foundation-hardening
> phase. No code was lost — only the version number was reset. The historical
> entries below (0.3.0, 0.2.x, etc.) are preserved as a record of feature
> introduction order, but the current shipping version is `0.1.0`.

---

## 0.1.0 — Foundation Reset (May 2026)

The features cataloged below (under 0.3.x and earlier) all ship in this
release; the renumbering is about commitment, not contents. Active work is
focused on security hardening, removing hardcoded limits, fixing the broken
MSVC RAII story, real parser error recovery, and a proper module/IR layer
before any new language feature lands.

See the [project roadmap](../roadmap/project-roadmap.md) for the path to 1.0.

---

## 0.3.x — Pre-reset (March 2026)

Post-0.3.0 additions:

- **Extended pattern matching** — `match` now works on `int`, `str`, and `bool`, with `_` wildcard
- **Defer statements** — `defer { ... }` for scope-based cleanup, LIFO execution order
- **Constants** — `const MAX: int = 100;` with compile-time evaluation
- **Type aliases** — `type ID = int;`
- **Do-while loops** — `do { ... } while cond;`
- **String and array method syntax** — `s.trim()`, `arr.push(x)`, `arr.len()`
- **Bitwise operators** — `&`, `|`, `^`, `~`, `<<`, `>>`, `&~`
- **Exponent and floored remainder** — `**` and `%%`
- **Mutable function parameters** — `fn foo(mut x: int)`
- **Raw emit** — `__emit__("C code");` for inline C
- **Standard library path** — `URUSCPATH` for module resolution
- Several bug fixes including security audit fixes, Termux build fix, and imported declaration warnings

## 0.3.0 — March 21, 2026

Major feature release:

- **Tuples** — `(int, str)` types with `.0`, `.1` access and destructuring
- **Runes (macros)** — `rune square(x) { x * x }`, invoked with `square!(5)`
- **If-expressions** — `let x = if cond { a } else { b };`
- **Type inference** — `let x = 42;` without explicit type annotations
- **HTTP built-ins** — `http_get(url)` and `http_post(url, body)`
- **String escape sequences** — `\t` and `\0` support
- Multiple security fixes in runtime (str_replace, pop, malloc checks, read_file)

## 0.2/3(A) — March 17, 2026

- **Struct spread syntax** — `Point { x: 10.0, ..p1 }`
- **Numeric separators** — `1_000_000`, `3.14_159`
- Bug fixes: parser infinite loop, empty struct literal, string `+=` codegen, trailing decimal float, sema column numbers, segfault on undefined function call

## 0.2/2(F) — March 9, 2026

- **CMake build system** replacing Makefile
- **Standalone compiler** — runtime embedded in the binary, no external files needed
- **Rich error diagnostics** — colored output with source context and caret pointer
- Cross-platform fixes: portable `fgets()`, binary-mode file writes, MSVC compatibility
- New CLI flags: `--help`, `--version`, `--tokens`, `--ast`
- Install/uninstall targets via CMake

## 0.2/1 — March 2, 2026

First feature-complete release:

- Enums with data variants and pattern matching
- String interpolation with `f"..."`
- Module system with `import` and circular import detection
- `Result<T, E>` type with `Ok`/`Err`/`unwrap`
- For-each loops over arrays
- Reference counting for automatic memory management
- Bounds-checked array access
- Comprehensive string operations
- File I/O built-ins

## 0.1 — Late 2025

Initial prototype:

- Primitive types: `int`, `float`, `bool`, `str`, `void`
- Variables with `let`/`let mut`
- Functions with typed parameters
- Structs and arrays
- Control flow: if/else, while, for-range, break, continue
- Basic operators and string concatenation
- Header-only runtime (`urus_runtime.h`)
- 30+ built-in functions
