<div align="center">
  <img src="https://raw.githubusercontent.com/Urus-Foundation/initial-resource/main/assets/logo.jpg" alt="URUS" width="140" />

  <h1>URUS Programming Language</h1>

  <p><strong>A statically-typed systems language that transpiles to portable C11.</strong><br/>Safe by default. Simple to read. Fast to run.</p>

  <p>
    <a href="https://github.com/Urus-Foundation/Urus/releases"><img alt="Version" src="https://img.shields.io/badge/version-0.1.0-blue?style=flat-square" /></a>
    <a href="./LICENSE"><img alt="License" src="https://img.shields.io/badge/license-Apache%202.0-green?style=flat-square" /></a>
    <img alt="Platform" src="https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows%20%7C%20Termux-lightgrey?style=flat-square" />
    <img alt="C Standard" src="https://img.shields.io/badge/output-C11-orange?style=flat-square" />
    <a href="./SECURITY.md"><img alt="Security" src="https://img.shields.io/badge/security-policy-informational?style=flat-square" /></a>
  </p>

  <p>
    <a href="#-quick-start">Quick Start</a> ·
    <a href="#-language-tour">Language Tour</a> ·
    <a href="./SPEC.md">Specification</a> ·
    <a href="./documentation/">Documentation</a> ·
    <a href="./examples/">Examples</a> ·
    <a href="./CHANGELOG.md">Changelog</a> ·
    <a href="#-roadmap">Roadmap</a>
  </p>
</div>

---

> [!IMPORTANT]
> URUS is in its **foundation-hardening phase** (v0.1.0). Public APIs and
> syntax are not yet frozen. We renumbered from 0.3.x → 0.1.0 deliberately —
> see [CHANGELOG.md](./CHANGELOG.md) for the full rationale. If you're
> evaluating URUS for production, please read [SECURITY.md](./SECURITY.md)
> and the roadmap below before committing.

---

## What is URUS?

URUS is a statically-typed programming language that compiles to standard
**C11**. It is designed to give you the safety of a modern language and the
portability of C — your code runs anywhere a C11 compiler runs, with no
runtime VM, no garbage collector, and no opaque dependencies.

The compiler is a single-pass transpiler written in C. It reads `.urus`
source files, performs lexing, parsing, semantic analysis, and emits portable
C11. The generated C is then handed to your system compiler (GCC, Clang, or
`clang-cl`) to produce a native binary.

### Why another language?

| Goal | How URUS gets there |
|------|---------------------|
| **Safer than C** | Reference-counted memory, runtime bounds checking, immutable by default, no pointer arithmetic in user code. |
| **Simpler than Rust** | No borrow checker, no lifetimes — a straightforward ownership model that the compiler manages for you. |
| **Faster than scripting languages** | Compiles to a native binary through C11. |
| **Portable by construction** | The compiler emits standard C — if a C11 compiler runs there, URUS runs there. |
| **Modern syntax without surprises** | Enums with payloads, pattern matching, string interpolation, `Result<T, E>`, tuples, defer, generics, traits (roadmap). |
| **Auditable** | The compiler is ~12k lines of C. You can read it. Every subsystem has been re-audited during the 0.1.0 foundation pass. |

---

## 🚀 Quick Start

### Requirements

| Tool | Minimum | Notes |
|------|---------|-------|
| C compiler | GCC 8+, Clang 10+, or `clang-cl` | Native MSVC is rejected at configure time — see [#199](https://github.com/Urus-Foundation/Urus/pull/199). |
| CMake | 3.10+ | |
| A C11 backend compiler | Any of the above | Used to compile the generated C to a native binary. |

### Build from source

```bash
git clone https://github.com/Urus-Foundation/Urus.git
cd Urus/compiler
cmake -S . -B build
cmake --build build
```

On **Termux**:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$PREFIX
cmake --build build
```

### Install (optional)

```bash
# Linux / macOS / Termux
sudo cmake --install build

# Windows (Administrator)
cmake --install build
```

### Hello, World

```rust
fn main(): void {
    print("Hello, World!");
}
```

```bash
urusc hello.urus -o hello
./hello
# Hello, World!
```

> [!TIP]
> Stuck on the build? Open a [bug report](https://github.com/Urus-Foundation/Urus/issues/new?template=complaint.md) or ask in the [WhatsApp community](https://chat.whatsapp.com/GYq9gBzXogU6U4JmiqV2dm?mode=gi_t).

---

## ✨ Language Tour

A whirlwind tour. The authoritative reference is [SPEC.md](./SPEC.md); the runnable examples live in [`examples/`](./examples/).

### Types

| Type | Description | C equivalent |
|------|-------------|--------------|
| `int` | 64-bit signed integer | `int64_t` |
| `float` | 64-bit floating point | `double` |
| `bool` | `true` / `false` | `bool` |
| `str` | UTF-8 string (heap, ref-counted) | `urus_str*` |
| `void` | No value | `void` |
| `[T]` | Dynamic array of `T` | `urus_array*` |
| `(T1, T2, ...)` | Tuple, stack-allocated | `struct { T1 f0; T2 f1; ... }` |
| `Result<T, E>` | `Ok(T)` or `Err(E)` | `urus_result*` |

### Variables and constants

```rust
let x: int = 10;            // immutable
let mut count: int = 0;     // mutable
count += 1;

// Type inference
let name = "hello";          // inferred as str
let pi = 3.14;               // inferred as float

const MAX_SIZE: int = 100;
const APP_NAME: str = "MyApp";

// Type aliases
type ID = int;
type Name = str;
```

### Functions

```rust
fn add(a: int, b: int): int {
    return a + b;
}

// Default parameter values
fn greet(name: str = "World"): void {
    print(f"Hello {name}!");
}

// Mutable parameters
fn increment(mut x: int): int {
    x += 1;
    return x;
}
```

### Tuples

```rust
let t: (int, str) = (42, "hello");
print(t.0);    // 42
print(t.1);    // hello

let (x, y) = get_pair();

let pairs: [(int, str)] = [(1, "a"), (2, "b")];
for (k, v) in pairs {
    print(f"{k}: {v}");
}
```

### Control flow

```rust
if x > 10        { print("big"); }
else if x > 5    { print("medium"); }
else             { print("small"); }

while x < 100 { x += 1; }

do { x += 1; } while x < 100;

for i in 0..10  { print(i); }     // exclusive range
for i in 0..=10 { print(i); }     // inclusive range

let names: [str] = ["Alice", "Bob"];
for name in names { print(name); }

// If-expression
let label = if x > 5 { "big" } else { "small" };
```

### Structs and enums

```rust
struct Point {
    x: float;
    y: float;
}

enum Shape {
    Circle(r: float);
    Rect(w: float, h: float);
    Empty;
}

fn area(s: Shape): float {
    match s {
        Shape.Circle(r)   => { return 3.14159 * r * r; }
        Shape.Rect(w, h)  => { return w * h; }
        Shape.Empty       => { return 0.0; }
    }
    return 0.0;
}
```

Match also works on primitives:

```rust
fn greet(lang: str): void {
    match lang {
        "en" => { print("Hello!"); }
        "id" => { print("Halo!"); }
        _    => { print("..."); }
    }
}
```

### Error handling

```rust
fn divide(a: int, b: int): Result<int, str> {
    if b == 0 {
        return Err("division by zero");
    }
    return Ok(a / b);
}

fn main(): void {
    let r = divide(10, 0);
    if is_err(r) {
        print(f"Error: {unwrap_err(r)}");
    } else {
        print(f"Result: {unwrap(r)}");
    }
}
```

### Strings and interpolation

```rust
let name: str = "World";
let count: int = 42;
print(f"Hello {name}! Answer: {count}");

let s: str = "  Hello World  ";
print(s.trim());            // "Hello World"
print(s.upper());           // "  HELLO WORLD  "
print(s.contains("World")); // true
print(s.len());             // 15
```

### Arrays

```rust
let nums: [int] = [1, 2, 3, 4, 5];
let first: int = nums[0];

let mut items: [int] = [];
items.push(42);
print(f"Length: {items.len()}");
```

### Modules

```rust
// math_utils.urus
fn square(x: int): int { return x * x; }

// main.urus
import "math_utils.urus";

fn main(): void {
    print(f"5^2 = {square(5)}");
}
```

### Defer

```rust
fn process(): void {
    print("start");
    defer { print("cleanup"); }
    print("working");
    // "cleanup" runs automatically at end of function
}
```

`defer` bodies execute in LIFO order and run before every return path.

### Runes (compile-time macros)

```rust
rune square(x) { x * x }
rune max(a, b) { if a > b { a } else { b } }

fn main(): void {
    print(square!(5));       // 25
    print(max!(10, 20));     // 20
}
```

### Operators

| Category | Operators |
|----------|-----------|
| Arithmetic | `+`, `-`, `*`, `/`, `%` |
| Exponent | `**` |
| Floored remainder | `%%` |
| Comparison | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Logical | `&&`, `\|\|`, `!` |
| Bitwise | `&`, `\|`, `^`, `~`, `<<`, `>>`, `&~` |
| Assignment | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=` |
| Increment / decrement | `++`, `--` |
| String concatenation | `+` |

---

## 📚 Built-in Functions

<details>
<summary><strong>I/O</strong></summary>

| Function | Description |
|----------|-------------|
| `print(value)` | Print to stdout with newline |
| `input()` | Read one line from stdin |
| `read_file(path)` | Read file contents as string |
| `write_file(path, s)` | Write string to file |
| `append_file(path, s)` | Append string to file |

</details>

<details>
<summary><strong>Array</strong></summary>

| Function | Description |
|----------|-------------|
| `len(array)` | Array length |
| `push(array, v)` | Append to array |
| `pop(array)` | Remove last element |

</details>

<details>
<summary><strong>String</strong></summary>

| Function | Method | Description |
|----------|--------|-------------|
| `str_len(s)` | `s.len()` | Length |
| `str_upper(s)` | `s.upper()` | Uppercase |
| `str_lower(s)` | `s.lower()` | Lowercase |
| `str_trim(s)` | `s.trim()` | Trim whitespace |
| `str_contains(s, sub)` | `s.contains(sub)` | Check substring |
| `str_find(s, sub)` | `s.find(sub)` | Find index of substring |
| `str_slice(s, a, b)` | `s.slice(a, b)` | Substring |
| `str_replace(s, old, new)` | `s.replace(old, new)` | Replace occurrences |
| `str_starts_with(s, p)` | `s.starts_with(p)` | Check prefix |
| `str_ends_with(s, p)` | `s.ends_with(p)` | Check suffix |
| `str_split(s, d)` | `s.split(d)` | Split into array |
| `char_at(s, i)` | `s.char_at(i)` | Character at index |

</details>

<details>
<summary><strong>Conversion, Math, Result, HTTP, Misc</strong></summary>

| Function | Description |
|----------|-------------|
| `to_str(v)`, `to_int(v)`, `to_float(v)` | Convert between types |
| `abs(x)`, `fabs(x)` | Absolute value |
| `sqrt(x)`, `pow(x, y)` | Square root, power |
| `min(a, b)`, `max(a, b)` | Min/max (int) |
| `fmin(a, b)`, `fmax(a, b)` | Min/max (float) |
| `is_ok(r)`, `is_err(r)` | Result discrimination |
| `unwrap(r)`, `unwrap_err(r)` | Result extraction (aborts on wrong variant) |
| `http_get(url)`, `http_post(url, body)` | HTTP requests (requires `curl`; see [security notes](./SECURITY.md#in-scope)) |
| `exit(code)`, `assert(cond, msg)` | Process control |

</details>

---

## 🛠️ CLI Usage

```
URUS Compiler 0.1.0

Usage: urusc <file.urus> [options]

Options:
  --help      Show help message
  --version   Show compiler version
  --tokens    Display lexer tokens
  --ast       Display the AST
  --emit-c    Print generated C to stdout (byte-deterministic)
  -o <file>   Output executable name (default: a.exe / a.out)

Example:
  urusc main.urus -o app
  urusc main.urus --emit-c > main.c
```

---

## 🏗️ Architecture

```
   .urus source
        │
        ▼
   ┌────────────┐
   │   Lexer    │  Tokenize source
   └────────────┘
        │
        ▼
   ┌──────────────────┐
   │  Preprocessor    │  Resolve imports (cycle-checked), expand runes
   └──────────────────┘
        │
        ▼
   ┌────────────┐
   │   Parser   │  Build AST (panic-mode error recovery)
   └────────────┘
        │
        ▼
   ┌────────────┐
   │    Sema    │  Type checking, scope analysis (O(1) lookup via FNV-1a)
   └────────────┘
        │
        ▼
   ┌────────────┐
   │  Codegen   │  Emit byte-deterministic C11
   └────────────┘
        │
        ▼
   ┌──────────────┐
   │  GCC / Clang │  Compile to native binary
   └──────────────┘
        │
        ▼
    Executable
```

For deeper architectural detail, see [`documentation/architecture/`](./documentation/architecture/) and the Architecture Decision Records under [`documentation/decisions/`](./documentation/decisions/).

---

## 📂 Project Structure

```
Urus/
├── compiler/
│   ├── lexer.c            # Tokenizer
│   ├── parser.c           # Recursive-descent parser, panic-mode recovery
│   ├── sema.c             # Type checking, scope analysis
│   ├── codegen.c          # C11 code generator (byte-deterministic)
│   ├── ast.c              # AST constructors and destructors
│   ├── preprocess.c       # Import resolution, rune expansion, cycle detection
│   ├── pkg.c              # Package manager (argv-spawned git)
│   ├── lsp.c              # LSP server
│   ├── urusc.c            # CLI driver (TOCTOU-safe temp files)
│   ├── misc.c             # x* allocators, file utilities
│   ├── runtime/
│   │   └── urus_runtime.h # Header-only runtime
│   ├── stdlib/            # Standard library modules (.urus)
│   └── CMakeLists.txt     # Build configuration (MSVC-rejecting guard)
├── examples/              # Sample programs
├── tests/
│   ├── run/               # Integration tests (.urus + .expected)
│   ├── invalid/           # Must-fail-to-compile tests
│   └── determinism.cmake  # Byte-determinism regression fixture
├── documentation/
│   ├── architecture/      # Subsystem deep-dives
│   ├── decisions/         # ADRs
│   ├── security/          # Security model and post-mortems
│   └── changelog/         # Historical release notes
├── SPEC.md                # Language specification
├── CHANGELOG.md           # Release history
├── README.md              # This file
├── SECURITY.md            # Security policy
├── CODE_OF_CONDUCT.md     # Contributor Covenant 2.1
├── CONTRIBUTING.md        # Contribution guide
├── Dockerfile             # Containerized build
└── LICENSE                # Apache License 2.0
```

---

## 📊 Project Stats

| Metric | Value |
|--------|-------|
| Version | 0.1.0 (foundation phase) |
| Compiler LOC | ~12,700 |
| Runtime LOC | ~540 (header-only) |
| Integration tests | 33+ run, multiple invalid-input |
| Output | Byte-deterministic C11 |
| Platforms | Linux, macOS, Windows (clang-cl), Termux |
| Build system | CMake 3.10+ |
| Dependencies (build) | A C11 compiler — that's it |
| Dependencies (runtime) | None for core language; `curl` only if you use `http_get`/`http_post` |
| CI | GCC + Clang × Linux/macOS/Windows + ASan/UBSan matrix |

---

## 🧪 Running Tests

```bash
cd compiler/build
ctest --output-on-failure
```

Every test is a `.urus` file in `tests/run/` paired with a `.expected` file
holding the program's expected stdout. The runner compiles the source, runs
it, and `diff`s the output. A subset is also re-compiled twice and SHA-256
compared to lock in codegen determinism (see [PR #201](https://github.com/Urus-Foundation/Urus/pull/201)).

For an AddressSanitizer + UndefinedBehaviorSanitizer pass:

```bash
cmake -S compiler -B build-asan -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g -O1"
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

---

## 🔬 How URUS Compares

> A high-level orientation table. None of these languages are strict
> competitors — they make different trade-offs. The point is to help you
> place URUS on the map.

| Feature | URUS | C | Rust | Go | Python |
|---------|:----:|:-:|:----:|:--:|:------:|
| Static typing | ✅ | ✅ | ✅ | ✅ | ❌ |
| Memory safety | RC + bounds | manual | ownership | GC | GC |
| Pattern matching | ✅ | ❌ | ✅ | ❌ | partial |
| String interpolation | ✅ | ❌ | ❌ | ❌ | ✅ |
| `Result<T, E>` | ✅ | ❌ | ✅ | ❌ | ❌ |
| Null safety | ✅ | ❌ | ✅ | ❌ | ❌ |
| Compiles to native | ✅ | ✅ | ✅ | ✅ | ❌ |
| Compiler complexity | small | small | large | medium | medium |
| Learning curve | low | medium | high | low | low |

---

## 🗺️ Roadmap

### v0.1.x — Foundation (current)

The 0.1.x line is a hardening pass over an already-feature-complete-ish
0.3.x codebase. The features below are **in** the language today; what the
0.1.0 release adds is auditability and resilience.

- [x] Stage 1 — critical & audit (8 PRs, see [CHANGELOG](./CHANGELOG.md))
  - x* allocator funneling, dynamic registries, sema hash index, package-manager argv spawn, LSP bounds checking, parallel-build race fixed, tuple-of-tuple miscompile fixed, license-header typo
- [x] Stage 2 — resilience & determinism (9 PRs)
  - TOCTOU temp files closed, emcc shell-free, ASan/UBSan CI matrix, parser panic-mode recovery, MSVC build guard, stdlib audit (ADR-004), byte-determinism regression test
- Already in the language: tuples & destructuring, runes, if-expressions, type inference, HTTP built-ins, `const`, `type` aliases, method-call syntax for strings and arrays, do-while, `defer`, extended pattern matching, bitwise & exponent operators, mutable parameters, raw `__emit__` blocks.

### v0.2.0 — Type System

- `Option<T>` and full null-safety story
- User-defined generics (`fn max<T: Ord>(a: T, b: T): T`)
- Portable RAII (explicit drop insertion, removes the GCC/Clang-only dependency)

### v0.3.0 — Methods, Traits, Closures

- `impl Point { fn distance() ... }`
- Trait / interface system
- First-class closures
- Bundled TCC as default backend (no system compiler needed)

### v1.0.0 — Stable Release

- Stable, frozen language surface
- Standard library with versioned modules
- Package manager (registry, lockfiles)
- Full documentation
- Production-ready

### v2.0.0 and beyond

- Async / await
- Concurrency primitives
- WebAssembly target (foundation in place; needs hardening)
- Self-hosting compiler
- LSP server matured to multi-file workspace support

---

## 💡 Design Inspiration

URUS borrows ideas from several places:

- **Rust** — enums with payloads, pattern matching, `Result`, immutability by default.
- **Go** — fast compilation, clean syntax, "simple by default" ergonomics.
- **Zig** — transpile-to-C philosophy, minimal runtime, audit-friendly compiler.
- **Python** — f-string interpolation, readability over cleverness.

We deliberately do **not** copy any of these languages wholesale; the goal is a coherent surface, not a polyfill.

---

## 🤝 Contributing

We welcome contributions of all kinds — bug reports, feature proposals,
documentation, tests, and code. The full guide is in
[CONTRIBUTING.md](./CONTRIBUTING.md); the abbreviated version:

1. **Fork** the repository on GitHub.
2. **Branch** from `main` with a typed prefix (`feat/`, `fix/`, `docs/`, ...).
3. **Make** your changes and add a test under `tests/run/` or `tests/invalid/`.
4. **Run** `ctest --output-on-failure` — all existing tests must still pass.
5. **Commit** using [Conventional Commits](https://www.conventionalcommits.org/) (`feat:`, `fix:`, `docs:`, ...).
6. **Open** a pull request against `main`.

> [!NOTE]
> The 0.1.0 foundation pass has audited out several patterns (raw `malloc`,
> hardcoded `MAX_*`, `system()`, predictable temp files, ...). Please don't
> reintroduce them; see the [Foundation-Phase Guardrails](./CONTRIBUTING.md#foundation-phase-guardrails) section.

All participation in the project is governed by our [Code of Conduct](./CODE_OF_CONDUCT.md) (Contributor Covenant 2.1).

---

## 🔒 Security

Found a vulnerability? **Please don't open a public issue.** See [SECURITY.md](./SECURITY.md) for our private reporting channels and response timeline. We aim to acknowledge reports within 72 hours.

---

## 📜 License

URUS is released under the [Apache License, Version 2.0](./LICENSE). You are free to use, modify, and distribute it — including in commercial projects — as long as you preserve the copyright notice and attribution. See [LICENSE](./LICENSE) for the full terms.

---

## 💬 Community

| Channel | What it's for |
|---------|---------------|
| [GitHub Discussions](https://github.com/Urus-Foundation/Urus/discussions) | Design questions, language proposals, "is this idiomatic?" |
| [GitHub Issues](https://github.com/Urus-Foundation/Urus/issues) | Bug reports, feature requests |
| [WhatsApp Community](https://chat.whatsapp.com/GYq9gBzXogU6U4JmiqV2dm?mode=gi_t) | Day-to-day chat, casual Q&A (Indonesian + English) |
| [SECURITY.md](./SECURITY.md) | Private security reports |

---

## 👥 Maintainers and Contributors

### Urus Foundation

<table>
  <tr>
    <td align="center"><a href="https://github.com/RasyaAndrean"><img src="https://github.com/RasyaAndrean.png" width="80" /><br /><sub><b>Rasya Andrean</b></sub></a><br /><sub>Founder &amp; Lead</sub></td>
    <td align="center"><a href="https://github.com/John-fried"><img src="https://github.com/John-fried.png" width="80" /><br /><sub><b>John-fried</b></sub></a><br /><sub>Co-Lead</sub></td>
    <td align="center"><a href="https://github.com/Mulyawan-ts"><img src="https://github.com/Mulyawan-ts.png" width="80" /><br /><sub><b>Mulyawan-ts</b></sub></a><br /><sub>Developer</sub></td>
  </tr>
</table>

### Contributors

<table>
  <tr>
    <td align="center"><a href="https://github.com/kkkfasya"><img src="https://github.com/kkkfasya.png" width="80" /><br /><sub><b>kkkfasya</b></sub></a></td>
    <td align="center"><a href="https://github.com/fmway"><img src="https://github.com/fmway.png" width="80" /><br /><sub><b>fmway</b></sub></a></td>
    <td align="center"><a href="https://github.com/fepfitra"><img src="https://github.com/fepfitra.png" width="80" /><br /><sub><b>fepfitra</b></sub></a></td>
    <td align="center"><a href="https://github.com/lordpaijo"><img src="https://github.com/lordpaijo.png" width="80" /><br /><sub><b>lordpaijo</b></sub></a></td>
    <td align="center"><a href="https://github.com/XBotzLauncher"><img src="https://github.com/XBotzLauncher.png" width="80" /><br /><sub><b>XBotzLauncher</b></sub></a></td>
    <td align="center"><a href="https://github.com/aimardcr"><img src="https://github.com/aimardcr.png" width="80" /><br /><sub><b>aimardcr</b></sub></a></td>
    <td align="center"><a href="https://github.com/billalxcode"><img src="https://github.com/billalxcode.png" width="80" /><br /><sub><b>billalxcode</b></sub></a></td>
    <td align="center"><a href="https://github.com/devirtz"><img src="https://github.com/devirtz.png" width="80" /><br /><sub><b>devirtz</b></sub></a></td>
  </tr>
</table>

Want to be listed? See [CONTRIBUTING.md](./CONTRIBUTING.md) and open a PR — every accepted contribution earns a spot.

---

<div align="center">
  <sub>Built with care by the Urus Foundation and contributors.</sub><br/>
  <a href="./documentation/">Documentation</a> ·
  <a href="./SPEC.md">Specification</a> ·
  <a href="./CHANGELOG.md">Changelog</a> ·
  <a href="./examples/">Examples</a> ·
  <a href="./SECURITY.md">Security</a>
</div>
