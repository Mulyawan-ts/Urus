# Contributing to URUS

Thank you for considering a contribution to URUS. Every well-reasoned PR — whether it's a one-line typo fix, a new language feature, a test case, or a clarification in the spec — moves the project forward.

URUS is currently in its **foundation-hardening phase** (v0.1.0). What that means for contributors is in the next section; please skim it before opening a non-trivial PR so we don't have to ask you to redo work.

---

## Table of Contents

1. [Project Phase: What "Foundation" Means for You](#project-phase-what-foundation-means-for-you)
2. [Getting Started](#getting-started)
3. [Ways to Contribute](#ways-to-contribute)
4. [Development Workflow](#development-workflow)
5. [Coding Standards](#coding-standards)
6. [Foundation-Phase Guardrails](#foundation-phase-guardrails)
7. [Testing](#testing)
8. [Adding a New Language Feature](#adding-a-new-language-feature)
9. [Adding a Standard Library Module](#adding-a-standard-library-module)
10. [Pull Request Guidelines](#pull-request-guidelines)
11. [Commit Message Format](#commit-message-format)
12. [Project Structure](#project-structure)
13. [Where to Ask for Help](#where-to-ask-for-help)
14. [License of Contributions](#license-of-contributions)

---

## Project Phase: What "Foundation" Means for You

We reset the version number from 0.3.x to **0.1.0** on purpose. The previous line shipped features faster than the foundation could absorb them: type system gaps, security holes in the package manager, broken RAII on MSVC, hardcoded limits that silently dropped declarations, and a parser without real error recovery.

So until the foundation pass closes:

- **Priority is correctness, safety, and audit-ability — not new features.** A PR that hardens an existing subsystem will land faster than a PR that adds a new one.
- **No public API or syntax is stable until 1.0.** Breaking changes are fine; just call them out in the PR description.
- **Every PR that touches the compiler must be reviewable in one sitting.** If your change is large, split it.
- **No silent failures.** If something can fail (allocation, parse, lookup, file open), it must produce a diagnostic the user can act on, or abort with a clear message — never return NULL into a caller that won't check.

The CHANGELOG describes which subsystems have already been audited; please don't reintroduce patterns we just removed (the [Foundation-Phase Guardrails](#foundation-phase-guardrails) section below enumerates them).

---

## Getting Started

### Prerequisites

| Tool | Minimum | Notes |
|------|---------|-------|
| **C compiler** | GCC 8+ or Clang 10+ | Native MSVC is **rejected at configure time** (see [#199](https://github.com/Urus-Foundation/Urus/pull/199)) — use `clang-cl` on Windows. |
| **CMake** | 3.10+ | |
| **Git** | 2.20+ | |
| **A C11 compiler for the *output*** | GCC / Clang / TCC | URUS transpiles to C11; you also need a backend compiler at run time. |

### Fork, Clone, Build

```bash
# 1. Fork on GitHub, then:
git clone https://github.com/<your-username>/Urus.git
cd Urus

# 2. Build the compiler
cd compiler
cmake -S . -B build
cmake --build build

# 3. Verify
./build/Debug/urusc.exe --version    # Windows
./build/urusc --version              # Linux / macOS / Termux
```

On **Termux**:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$PREFIX
cmake --build build
```

If the build fails with `Urus v0.1.0 cannot be built with native MSVC`, you're hitting the intentional guard — switch to GCC, Clang, or `clang-cl`.

### Run the Test Suite

```bash
cd compiler/build
ctest --output-on-failure
```

Every test is a `.urus` file in `tests/run/` paired with a `.expected` file. The runner compiles the program, executes it, and compares the output byte-for-byte. A subset is also re-run twice and SHA-256 compared to lock in codegen determinism (see [#201](https://github.com/Urus-Foundation/Urus/pull/201)).

---

## Ways to Contribute

### Report a Bug

1. Search [existing issues](https://github.com/Urus-Foundation/Urus/issues) first.
2. Open a new issue using the bug template, including:
   - `urusc --version` output.
   - OS and backend C compiler version.
   - A **minimal** `.urus` file that reproduces the bug — please trim away everything that isn't needed.
   - Expected vs. observed behavior (the actual error message, not a summary).
   - The shortest reproducer wins; don't paste a 500-line program if 20 lines triggers the same bug.

### Suggest a Feature

1. Open an issue with the `enhancement` label.
2. Describe the use case before the syntax. We are deliberately conservative about adding syntax during the foundation phase; a strong use case is the deciding factor.
3. If your proposal affects the language surface, **don't implement it before the design is agreed on** — that saves us both work.

### Improve Documentation

The fastest accepted PRs are usually doc fixes. README, SPEC, examples, and `documentation/` are all in scope. Run a spellchecker before opening the PR.

### Report a Security Vulnerability

**Don't open a public issue.** See [SECURITY.md](./SECURITY.md) for the private reporting channels and response timeline.

---

## Development Workflow

```bash
# 1. Sync your fork
git checkout main
git pull origin main

# 2. Branch
git checkout -b feat/your-feature-name      # or fix/, refactor/, docs/, test/, chore/

# 3. Implement + add a test
#    - Compiler change → tests/run/<name>.urus + tests/run/<name>.expected
#    - Invalid-input case → tests/invalid/<name>.urus (must fail to compile)

# 4. Build and run the full suite
cd compiler && cmake --build build
cd build && ctest --output-on-failure

# 5. If your change touches codegen, also run a sanitizer pass locally:
cmake -S .. -B build-asan -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g"
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure

# 6. Commit (see Commit Message Format below)
git commit -m "feat(parser): support N"

# 7. Push and open a PR against main
git push -u origin feat/your-feature-name
```

### Branch naming

| Prefix | Use for |
|--------|---------|
| `feat/` | New language feature, stdlib module, or user-visible capability |
| `fix/` | Bug fix |
| `refactor/` | Internal restructure, no behavior change |
| `docs/` | Documentation only |
| `test/` | Adding or restructuring tests |
| `chore/` | Build system, CI, tooling |
| `perf/` | Performance improvements |
| `security/` | Security-sensitive change (will be reviewed more carefully) |

---

## Coding Standards

### C Code (Compiler)

| Rule | Standard |
|------|----------|
| **Language** | C11 (`-std=c11`). Don't use compiler-specific extensions without a fallback. |
| **Indentation** | 4 spaces, no tabs. |
| **Line length** | 80 characters preferred; up to 100 is acceptable for code that is genuinely clearer on one line. |
| **Naming** | `snake_case` for functions and variables; module prefix for public symbols (`lexer_`, `sema_`, `codegen_`). |
| **Warnings** | Must compile clean under `-Wall -Wextra -Wpedantic`. CI runs both GCC and Clang. |
| **Sanitizers** | Must pass the AddressSanitizer + UndefinedBehaviorSanitizer matrix job ([#196](https://github.com/Urus-Foundation/Urus/pull/196)). |
| **No dynamic allocation outside `x*` wrappers** | See [Foundation-Phase Guardrails](#foundation-phase-guardrails). |

**Brace style** — custom Mozilla/K&R hybrid:

```c
/* Function: brace on a new line */
void
lexer_advance(Lexer *l)
{
    /* Control flow: brace on the same line */
    if (l->pos >= l->len) {
        return;
    }
    l->pos++;
}

/* Struct/Enum: brace on the same line */
typedef struct {
    int pos;
    int len;
} Lexer;
```

No single-line blocks. `if`, `for`, `while`, `do` always use braces — even for a single-line body. This is non-negotiable; we have already had bugs caused by missing braces in patches.

### URUS Code (Examples, Stdlib, Tests)

```rust
fn main(): void {
    let name: str = "World";
    print(f"Hello {name}!");
}
```

- Semicolons terminate statements.
- `let` for immutable, `let mut` for mutable.
- Explicit return types on functions (`: void` for no return).
- Prefer pattern matching over chained `if/else` when matching tagged data.

---

## Foundation-Phase Guardrails

These patterns have been audited out of the codebase during the 0.1.0 foundation pass. **Do not reintroduce them.** Each line links to the PR where the audit happened so you can see the rationale.

### Allocation

- **No raw `malloc` / `calloc` / `realloc` / `strdup` in compiler-internal code.** Use `xmalloc` / `xcalloc` / `xstrdup` / `xrealloc` from `compiler/misc.c`. They abort on OOM with a clear message; the caller never has to NULL-check. ([#192](https://github.com/Urus-Foundation/Urus/pull/192))
- **Runtime code (`compiler/runtime/`) intentionally stays on checked-malloc.** User programs may want to handle OOM themselves; do not change this without an ADR.

### Limits and registries

- **No new hardcoded `MAX_*` caps.** Every registry that can grow under user input must use the `xrealloc`-backed grow-by-2 scheme starting at 16. ([#188](https://github.com/Urus-Foundation/Urus/pull/188), [#190](https://github.com/Urus-Foundation/Urus/pull/190))
- **No static buffers for identifier names.** Tuple type names, generic instantiation names, etc. must return heap-owned strings the caller `xfree`s. ([#193](https://github.com/Urus-Foundation/Urus/pull/193))

### External processes

- **No `system()` calls anywhere in the compiler.** External binaries (`git`, `emcc`, etc.) are invoked via `fork`/`execvp` (POSIX) or `_spawnvp` (Windows) with `argv` arrays — no shell in the loop. ([#187](https://github.com/Urus-Foundation/Urus/pull/187), [#194](https://github.com/Urus-Foundation/Urus/pull/194))
- **Any user-controlled string passed to a subprocess must be validated against a strict allowlist first.** See `compiler/stdlib/http.urus` for an example. ([#200](https://github.com/Urus-Foundation/Urus/pull/200))

### Filesystem

- **No predictable temp file names.** Use `mkstemps` (POSIX) or `_sopen_s` + `_O_CREAT | _O_EXCL` (Windows). ([#194](https://github.com/Urus-Foundation/Urus/pull/194))
- **Validate every offset read from a URI, path, or external buffer before using it.** See `lsp.c` for the canonical pattern. ([#186](https://github.com/Urus-Foundation/Urus/pull/186))

### Build / portability

- **Native MSVC is not supported.** The CMake configure step refuses to proceed; if you need Windows builds use `clang-cl` (we depend on `__attribute__((cleanup))` for RAII). ([#199](https://github.com/Urus-Foundation/Urus/pull/199))
- **Codegen output must be byte-deterministic for the same input.** A CTest fixture asserts this on six representative fixtures. If your change makes determinism conditional on iteration order, environment, or PID, it will fail in CI. ([#201](https://github.com/Urus-Foundation/Urus/pull/201))

### Parser / error reporting

- **A single syntax error must not suppress diagnostics for the rest of the file.** The parser uses panic-mode recovery — synchronize on the next top-level declaration boundary. Don't add new error paths that swallow further errors. ([#198](https://github.com/Urus-Foundation/Urus/pull/198))

---

## Testing

### What counts as a test

Every behavior change needs at least one test. For the compiler, a test is a `.urus` source file plus a sibling `.expected` file containing the program's expected stdout. The CTest runner compiles the source, runs the executable, and `diff`s the output.

```
tests/run/my_feature.urus       # source
tests/run/my_feature.expected   # expected stdout
```

For an invalid-input case (the compiler should refuse to compile), put the source in `tests/invalid/`:

```
tests/invalid/my_bad_input.urus     # must fail to compile
```

### Determinism fixtures

Six tests are also wired into `tests/determinism.cmake` and re-compiled twice with SHA-256 verification: `hello`, `tuples`, `closures`, `generics`, `nested_tuple_typedef`, `many_lambdas`. If your change introduces non-determinism (e.g. printing a pointer address into a generated identifier), one of these will start failing. Don't suppress the failure — fix the source of non-determinism.

### Local sanitizer runs

CI runs ASan + UBSan, but it's much faster to catch issues locally:

```bash
cmake -S compiler -B build-asan -DCMAKE_C_FLAGS="-fsanitize=address,undefined -g -O1"
cmake --build build-asan
cd build-asan && ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest
```

---

## Adding a New Language Feature

Follow this checklist top-to-bottom. Skipping a step (especially the test) is the most common reason a PR gets bounced.

| Step | File(s) | Notes |
|------|---------|-------|
| 1. Add the token | `compiler/lexer.h` (TokenType enum), `compiler/lexer.c` | Pick a name that won't conflict with future syntax. |
| 2. Recognize the token in the lexer | `compiler/lexer.c` | Make sure the keyword lookup table stays sorted if you're adding a keyword. |
| 3. Add the AST node | `compiler/ast.h` + `compiler/ast.c` | Use `xmalloc` / `xcalloc`. Add a `drop_*` function if the node owns heap data. |
| 4. Parse the syntax | `compiler/parser.c` | Make sure the new path participates in panic-mode recovery — don't bypass `error_at()`. |
| 5. Type-check | `compiler/sema.c` | Always emit a diagnostic on rejection, never silently fall through. |
| 6. Generate C | `compiler/codegen.c` | Generated identifiers must be deterministic. Use the existing `tmp_counter` for fresh names. |
| 7. Runtime support | `compiler/runtime/urus_runtime.h` | Only if the feature needs a new runtime primitive. Keep the runtime header-only. |
| 8. Add a positive test | `tests/run/<name>.urus` + `.expected` | Exercise the happy path. |
| 9. Add a negative test | `tests/invalid/<name>.urus` | Confirm the type checker rejects misuse. |
| 10. Update the spec | `SPEC.md` | Add the section before opening the PR — the spec is the contract. |
| 11. Add an example | `examples/<name>.urus` | A short, illustrative `.urus` file users can copy-paste. |
| 12. Update CHANGELOG | `CHANGELOG.md` | Under the current `## Unreleased` section. Link your PR after it lands. |

---

## Adding a Standard Library Module

Standard library modules live in `compiler/stdlib/` as `.urus` files.

1. Create `compiler/stdlib/<your_module>.urus`.
2. Prefix every exported function with the module name (e.g. `math_sin`, `json_parse`). The stdlib has no namespace separation; the prefix is the namespace.
3. Use `__emit__()` for C-level bindings — but **validate every input first**. See `compiler/stdlib/http.urus` for the validator pattern and [ADR-004](./documentation/decisions/ADR-004-stdlib-audit-v0.1.0.md) for the rationale.
4. Add test cases under `tests/run/stdlib_<module>_*.urus`.
5. Document the module under "Standard Library" in `SPEC.md`.
6. If the module shells out to an external binary, **do not** use `popen` / `system`; use the `argv`-style spawn helpers already in `urusc.c`.

---

## Pull Request Guidelines

- **Target `main`.** All feature branches merge into `main`; there is no separate develop branch.
- **One concern per PR.** A PR that mixes a refactor with a new feature is harder to review and harder to revert.
- **Describe what and why.** The PR body should explain the user-visible change and the reason for the design. If you considered an alternative and rejected it, mention it — that saves the reviewer asking.
- **Link related issues.** `Closes #123`, `Refs #456`.
- **Tests pass before review.** CI must be green; if it isn't, mark the PR as draft until it is.
- **Update docs in the same PR.** If your change is user-visible, SPEC/README/CHANGELOG updates land together. We don't accept "docs in a follow-up" anymore.
- **Keep the diff minimal.** Don't reformat files you didn't otherwise touch; a stray whitespace change makes `git blame` worse for everyone.
- **Sign off your commits.** Use `git commit -s` if your contribution requires a DCO; we don't require it today, but the option is there for downstream forks that do.

A PR is ready to merge when:

1. CI is green (build + tests + sanitizers + determinism).
2. At least one maintainer has approved.
3. All review comments are resolved (either fixed or explicitly deferred to a follow-up issue).
4. The CHANGELOG entry is in place.

---

## Commit Message Format

We use [Conventional Commits](https://www.conventionalcommits.org/). The type prefix is required.

| Type | Use for |
|------|---------|
| `feat` | New user-visible feature |
| `fix` | Bug fix |
| `refactor` | Internal change with no behavior difference |
| `docs` | Documentation only |
| `test` | New or restructured tests |
| `chore` | Build, CI, tooling, dependency bumps |
| `perf` | Performance improvement |
| `security` | Security-sensitive change |

Examples (from real merged PRs):

```
feat(parser): panic-mode top-level error recovery (#198)
fix(stdlib/http): tighten shell-injection allowlist; document audit (#200)
chore(runtime): refuse to build under native MSVC (#199)
test: assert urusc --emit-c is byte-deterministic across runs (#201)
```

**Commit body.** If the subject line isn't enough, add a body explaining the motivation, the trade-off you considered, or the failure mode you fixed. Wrap at 72 columns.

---

## Project Structure

```
Urus/
├── compiler/
│   ├── lexer.c            # Tokenizer
│   ├── parser.c           # Recursive descent parser (panic-mode recovery)
│   ├── sema.c             # Type checking and semantic analysis
│   ├── codegen.c          # C11 code generator
│   ├── ast.c              # AST constructors and drop functions
│   ├── preprocess.c       # Import resolution + rune (macro) expansion
│   ├── pkg.c              # Package manager (argv-based git invocation)
│   ├── lsp.c              # LSP server (bounds-checked URI parsing)
│   ├── urusc.c            # CLI driver (TOCTOU-safe temp files, argv-spawn)
│   ├── misc.c             # x* allocators, file utilities
│   ├── runtime/
│   │   └── urus_runtime.h # Header-only runtime (strings, arrays, results)
│   ├── stdlib/            # Standard library modules (.urus)
│   │   ├── http.urus      # HTTP client (validator-hardened)
│   │   ├── json.urus
│   │   └── ...
│   └── CMakeLists.txt     # Build configuration + MSVC guard
├── examples/              # Sample programs
├── tests/
│   ├── run/               # Integration tests (.urus + .expected)
│   ├── invalid/           # Must-fail-to-compile tests
│   └── determinism.cmake  # Byte-determinism CTest fixture
├── documentation/         # Extended documentation
│   ├── decisions/         # Architecture Decision Records (ADRs)
│   ├── security/          # Security model and post-mortems
│   └── changelog/         # Historical version notes
├── SPEC.md                # Language specification
├── CHANGELOG.md           # Release history
├── README.md              # Project overview
├── SECURITY.md            # Security policy
├── CODE_OF_CONDUCT.md     # Community standards (Contributor Covenant 2.1)
├── CONTRIBUTING.md        # This file
├── Dockerfile             # Containerized build
└── LICENSE                # Apache License 2.0
```

> The compiler is **flat on purpose**. There is no `src/` / `include/` split; every translation unit lives next to its peers in `compiler/`. If you see references to `compiler/src/` in an older doc, that's stale — please send a docs PR.

---

## Where to Ask for Help

- **General usage / syntax questions** — open a [Discussion](https://github.com/Urus-Foundation/Urus/discussions) or ask in the [WhatsApp community](https://chat.whatsapp.com/GYq9gBzXogU6U4JmiqV2dm?mode=gi_t).
- **"Is this a bug?"** — open an issue with a minimal reproducer; we'd rather see a too-small report than no report.
- **"Is this design idea reasonable?"** — open a Discussion thread before sinking time into the implementation. Save yourself a rewrite.
- **Documentation pointers:**
  - [README](./README.md) — overview and quick start
  - [SPEC](./SPEC.md) — language reference
  - [Documentation index](./documentation/index.md) — architecture, security, ADRs

---

## License of Contributions

By submitting a contribution, you agree that it will be licensed under the [Apache License, Version 2.0](./LICENSE), the same license that covers the rest of URUS. You retain copyright; the license is what makes it usable by everyone.

If your employer has rights to your contributions, please check that you're authorized to contribute before opening the PR.

---

Thank you again. URUS is a small project with a clear goal — a safer, simpler systems language that compiles down to portable C — and every contribution that helps us reach 1.0 is appreciated.
