# Roadmap

## Version History

| Version | Status | Date | Highlight |
|---------|--------|------|-----------|
| v0.1 | Released | 2025 | Initial prototype — lexer, parser, basic codegen |
| v1.0.0 | Released | 2026 | Enums, match, imports, Result, f-strings, refcounting |

## Current: v1.0.0

Features included:
- Primitive types: `int`, `float`, `bool`, `str`, `void`
- Variables (immutable by default, `mut` for mutable)
- Functions
- Structs
- Arrays (dynamic, growable)
- Enums / tagged unions
- Pattern matching (`match`)
- String interpolation (`f"..."`)
- Modules / imports
- For-each loops (`for item in array { ... }`)
- Error handling (`Result<T, E>`, `Ok`, `Err`)
- Reference counting memory management
- File I/O
- Full test suite

---

## Planned: v1.1.0 — Quality of Life

**Target:** Stability and developer experience

| Feature | Description | Priority |
|---------|-------------|----------|
| Default parameter values | `fn foo(x: int = 10)` | Medium |
| Multi-line string literals | Triple-quote `"""..."""` | Medium |
| Better error messages | Show source context, suggestions | High |
| Warning system | Non-fatal warnings (unused vars, etc.) | Medium |
| `--version` flag | Print version number | Low |

---

## Planned: v1.2.0 — Type System

**Target:** More expressive type system

| Feature | Description | Priority |
|---------|-------------|----------|
| Type aliases | `type ID = int;` | Medium |
| Optional type | `Option<T>` (sugar for `Result<T, void>`) | High |
| Tuple types | `(int, str)` — anonymous product types | Medium |
| Type inference | `let x = 42;` (infer `int`) | High |
| Const expressions | `const PI: float = 3.14159;` | Low |

---

## Planned: v2.0.0 — Major Features

**Target:** Production-ready language

| Feature | Description | Priority |
|---------|-------------|----------|
| Methods | `impl Point { fn distance(...) }` | High |
| Traits/Interfaces | `trait Printable { fn to_string(): str; }` | High |
| Generics | `fn max<T>(a: T, b: T): T` | High |
| Closures | `let f = \|x\| x * 2;` | Medium |
| Standard library | `std.collections`, `std.io`, `std.math` | High |
| Package manager | Download & manage dependencies | Medium |

---

## Future: v3.0.0 — Advanced

| Feature | Description |
|---------|-------------|
| Async/await | Asynchronous I/O |
| Concurrency | Lightweight threads / goroutine-style |
| Cycle detection | Detect ref-count cycles at runtime |
| Compile-time evaluation | `comptime` blocks |
| WASM target | Compile to WebAssembly |
| Self-hosting | Compiler written in URUS |
| LSP server | Language Server Protocol for IDE support |
| Debugger integration | Source-level debugging |

---

## Milestones

```
2025 Q4  ── v0.1 prototype (done)
2026 Q1  ── v1.0.0 release (done)
2026 Q2  ── v1.1.0 quality of life
2026 Q3  ── v1.2.0 type system improvements
2026 Q4  ── v2.0.0 planning & design
2027 H1  ── v2.0.0 methods, traits, generics
2027 H2  ── v2.x standard library & tooling
2028     ── v3.0.0 async, concurrency, WASM
```

## Contributing

Want to contribute? See the [Development Guide](../development-guide/README.md) for:
- Coding standards
- Branch strategy
- Testing guidelines
- How to add new features
