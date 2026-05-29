# Project Roadmap

This document tracks what has been released, what's currently in progress, and what's planned for future versions.

## Release history

| Version | Date | Highlights |
|---------|------|-----------|
| 0.1 (prototype) | 2025 Q4 | Initial prototype — lexer, parser, basic codegen, primitive types |
| 0.2/1 | 2026-03-02 | Enums, pattern matching, imports, Result type, string interpolation, ref-counting |
| 0.2/2(F) | 2026-03-09 | CMake build, standalone compiler, rich error diagnostics, cross-platform fixes |
| 0.2/3(A) | 2026-03-17 | Struct spread syntax, numeric separators, multiple bug fixes |
| 0.3.0 | 2026-03-21 | Tuples, runes (macros), if-expressions, type inference, HTTP built-ins |
| 0.3.x | 2026-03 — 2026-05 | Constants, type aliases, defer, extended match, bitwise ops, method syntax, do-while, raw emit, mutable params, stdlib path |
| **0.1.0** (reset) | **2026-05** | **Foundation-hardening reset. 17 PRs across two stages — audit, security, resilience, determinism. See [CHANGELOG](../../CHANGELOG.md).** |

> **Why the version went backwards.** The 0.3.x line accumulated features faster than the foundation could absorb them — type system gaps, security holes in the package manager, broken RAII on MSVC, hardcoded limits that silently dropped declarations, and a parser without real error recovery. We restarted the numbering to make it loud that **no public API is stable until 1.0**. All previously-shipped features were kept; what changed is the commitment to audit them.

## Current state: 0.1.0 (foundation phase)

All features delivered in the 0.2/0.3 lines are still in the language. What 0.1.0 adds is auditability and resilience:

- Primitive types (`int`, `float`, `bool`, `str`, `void`) and composite types (arrays, tuples, Result, structs, enums)
- Type inference for local variables
- Compile-time constants and type aliases
- Functions with default parameters and mutable parameters
- Full control flow: if/else, while, do-while, for-range, for-each, break, continue
- If-expressions (ternary-style)
- Pattern matching on enums, int, str, and bool
- Tuples with destructuring in let and for-each
- Runes (compile-time macros)
- Defer statements
- String interpolation
- Method-call syntax for strings and arrays
- Automatic memory management via reference counting
- Modules and imports with library path support (`URUSCPATH`)
- Error handling with `Result<T, E>`
- HTTP built-ins (`http_get`, `http_post`)
- Raw emit for inline C code
- Bitwise operators, exponentiation (`**`), floored remainder (`%%`)
- Increment/decrement operators
- Numeric literals: hex, octal, binary, separators, scientific notation
- 33 integration tests, Dockerfile, CMake build, cross-platform support

**Foundation-pass hardening landed in 0.1.0:**

- All compiler allocations funnel through `xmalloc` / `xcalloc` / `xstrdup` / `xrealloc` (abort-on-OOM)
- All hardcoded `MAX_*` caps removed; tables grow dynamically
- Symbol-table lookup is now amortized O(1) via FNV-1a hash index
- Package manager invokes `git` via `argv` spawn (no shell); URL allowlist
- TOCTOU temp-file vector closed (`mkstemps` / `O_EXCL`)
- emcc / WASM backend invoked via `_spawnvp` / `execvp` (no `system()`)
- Preprocessor detects and reports circular imports
- LSP `uri_to_path` bounds-checked
- Tuple-of-tuple silent miscompile fixed; tuple type names now heap-owned
- Parser panic-mode error recovery
- Native MSVC build refused at configure time (CMake guard)
- AddressSanitizer + UndefinedBehaviorSanitizer matrix in CI
- Stdlib `http` validator hardened ([ADR-004](../decisions/ADR-004-stdlib-audit-v0.1.0.md))
- Codegen byte-determinism regression test

## Planned: 0.2.0 — Type system expansion

The next major release focuses on making the type system more expressive.

| Feature | Description |
|---------|-------------|
| `Option<T>` | Optional values — a cleaner way to represent "might not exist" than `Result<T, void>` |
| Generics | Parameterized types and functions: `fn max<T>(a: T, b: T): T` |
| Portable RAII | Explicit drop insertion for more predictable cleanup, especially for structs |

## Planned: 0.3.0 — Methods and traits

This release introduces object-oriented capabilities while keeping the language simple.

| Feature | Description |
|---------|-------------|
| Methods | `impl Point { fn distance(self): float { ... } }` — attach functions to types |
| Traits | `trait Display { fn to_string(self): str; }` — shared interfaces |
| Closures | Anonymous functions: `let f = \|x\| x * 2;` |
| Bundled TCC | Ship TCC as the default C backend to remove the GCC dependency |

## Planned: 1.0.0 — Stable release

The 1.0 release marks the point where the language is production-ready and we commit to backward compatibility.

| Feature | Description |
|---------|-------------|
| Standard library | Curated modules for collections, I/O, math, and string processing |
| Package manager | Dependency management and a package registry |
| Complete documentation | A polished language guide, API reference, and tutorial |
| Stability guarantee | No breaking changes within the 1.x series |

## Future: 2.0.0 — Advanced features

These are longer-term goals that may require significant compiler changes.

| Feature | Description |
|---------|-------------|
| Async/await | Non-blocking I/O without callback hell |
| Concurrency | Lightweight threads or goroutine-style concurrency |
| WebAssembly target | Compile URUS programs to run in the browser |
| Self-hosting | Rewrite the compiler in URUS |
| LSP server | Language Server Protocol support for IDE integration |

## Timeline

```
2025 Q4  ── 0.1 prototype                              (done)
2026 Q1  ── 0.2.x core features + stabilization        (done, pre-reset)
2026 Q1  ── 0.3.x tuples, macros, defer, etc.          (done, pre-reset)
2026 Q2  ── 0.1.0 foundation reset (17 audit PRs)      (done)
2026 Q3  ── 0.2.0 type system expansion (post-reset)
2026 Q4  ── 0.3.0 methods, traits, closures (post-reset)
2027 H1  ── 1.0.0 stable release
2027 H2  ── 1.x standard library and tooling
2028     ── 2.0.0 async, concurrency, WASM
```

These dates are targets, not commitments. The project is maintained by a small team and the pace depends on contributor availability.

## Contributing

If any of these planned features interests you, check the [issue tracker](https://github.com/Urus-Foundation/Urus/issues) for related issues, or open a new one to discuss your approach before starting work. The [Contributor Guide](../development-guide/contributor-guide.md) explains the workflow.
