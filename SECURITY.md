# Security Policy

URUS is a young language in its **foundation-hardening phase**. We are renumbering to 0.1.0 precisely because we are auditing every subsystem before adding new features. Security reports are first-class work for us; this document describes how to send one, what we'll do with it, and what we consider in or out of scope.

---

## Supported Versions

| Version | Status | Notes |
|---------|--------|-------|
| 0.1.x | **Actively supported** | Current foundation line. Security and correctness fixes land here first. |
| 0.3.x | End of life | Pre-reset release line. No further fixes. See [version-history](./documentation/changelog/version-history.md). |
| 0.2.x | End of life | Pre-reset release line. No further fixes. |

> The 0.3.x → 0.1.0 version reset is intentional. The 0.3.x line accumulated features faster than the foundation could absorb them; we restarted the version number to signal that **no public API or syntax is stable until 1.0**. See [CHANGELOG.md](./CHANGELOG.md) for the full rationale.

---

## Reporting a Vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.** Public disclosure before a fix is available puts every user at risk. Use one of the private channels below instead.

### Private channels

| Channel | Address | Latency |
|---------|---------|---------|
| Email (preferred) | rasyaandrean@outlook.co.id | Within 72 hours |
| GitHub private vulnerability report | https://github.com/Urus-Foundation/Urus/security/advisories/new | Within 72 hours |
| GitHub DM to maintainer | [@RasyaAndrean](https://github.com/RasyaAndrean) | Best-effort |

### What to include

A good report makes our job — and yours — much faster:

1. **A clear title** describing the class of bug (e.g. "TOCTOU on temp file in `urusc` driver").
2. **Affected version** (`urusc --version` output) and the platform (Windows / Linux / macOS / Termux).
3. **A minimal reproducer** — ideally a `.urus` file or a short command line — that triggers the issue.
4. **Expected vs. observed behavior**, and your assessment of impact (RCE, info-disclosure, DoS, etc.).
5. **Suggested mitigation** if you have one. (Not required, but appreciated.)
6. **Disclosure preference** — would you like to be credited, credited under a handle, or stay anonymous?

If a fix is non-trivial, please give us a reasonable embargo window (we'll discuss timing in our first reply).

---

## Our Response Process

| Stage | Target | Description |
|-------|--------|-------------|
| **Acknowledgement** | Within 72 hours | Confirm receipt, assign a tracking ID, ask for missing repro details if needed. |
| **Severity triage** | Within 1 week | Score the issue (see severity table below), decide whether to embargo, identify the affected subsystem(s). |
| **Fix development** | Severity-dependent | Critical: days. High: 1–2 weeks. Medium: bundled with the next foundation PR. |
| **Coordinated disclosure** | After fix lands | We publish a GitHub Security Advisory (GHSA), cut a patch release, and credit the reporter (unless they opt out). |
| **Public post-mortem** | Within 2 weeks of disclosure | For Critical/High issues, we publish a short post-mortem in `documentation/security/` describing the root cause and the test that now guards against it. |

### Severity rubric

| Severity | Examples |
|----------|----------|
| **Critical** | Arbitrary code execution via crafted input, supply-chain RCE in package manager, runtime memory corruption with attacker-controlled write. |
| **High** | Privilege boundary bypass, sandbox escape, deterministic crash on common input that produces a corrupted binary. |
| **Medium** | Denial-of-service via pathological input, leak of local file paths, error message containing sensitive context. |
| **Low** | Hardening opportunity, defense-in-depth, missing input validation that has no current exploit path. |

---

## Scope

### In scope

| Area | Examples |
|------|----------|
| **Compiler driver** (`urusc`) | Buffer overflow, crash on crafted `.urus` input, arbitrary code execution via crafted source, TOCTOU on temporary files. |
| **Preprocessor / import resolver** | Path traversal via `import`, unbounded recursion, symlink-following surprises. |
| **Parser / Sema / Codegen** | Crash on adversarial input, type confusion, generated C that miscompiles in a way an attacker can leverage. |
| **Runtime** (`urus_runtime.h`) | Memory corruption, bounds-check bypass, reference-count manipulation, use-after-free. |
| **Generated C** | Codegen producing unsafe C (missing bounds check, missing free, signed overflow that the user did not write). |
| **Standard library** (`compiler/stdlib/`) | Shell injection in `http`, request smuggling, unsafe URL handling. See [ADR-004](./documentation/decisions/ADR-004-stdlib-audit-v0.1.0.md). |
| **Package manager** | Dependency confusion, supply-chain RCE through `urus.toml`, unsafe `git clone` targets. (Hardened in 0.1.0 — see CHANGELOG.) |
| **LSP server** (`urusc-lsp`) | Crash on malformed LSP request, URI parsing buffer overruns. |

### Out of scope

The following are **not** treated as URUS vulnerabilities:

- Vulnerabilities in GCC, Clang, MSVC, or any backend C compiler.
- Vulnerabilities in `curl` (used by the `http` stdlib) — please report those to the curl project.
- Logic bugs in user-written URUS programs (e.g. the user's own division-by-zero).
- Denial of service via deliberately large inputs (a 10 GB `.urus` file is expected to exhaust memory; this is a known limitation tracked outside the security tracker).
- Issues that require an attacker who already has full local control of the developer machine (e.g. "I can edit your source files, therefore RCE").
- Cosmetic issues in error messages.

If you're not sure whether something is in scope, **report it anyway** and we'll triage.

---

## Security Posture (v0.1.0)

The foundation pass has actively hardened the following surfaces. We list them so reporters know what's already been examined — finding a regression here is high-signal.

### Memory safety

| Mitigation | Where | Landed |
|------------|-------|--------|
| All compiler-internal allocations funnel through `xmalloc` / `xcalloc` / `xstrdup` / `xrealloc` (abort-on-OOM, no NULL-deref) | `compiler/misc.c` | [#192](https://github.com/Urus-Foundation/Urus/pull/192) |
| Latent `__xrealloc` bug fixed (was returning new pointer but not writing it back through `void**`) | `compiler/misc.c` | [#192](https://github.com/Urus-Foundation/Urus/pull/192) |
| AddressSanitizer + UndefinedBehaviorSanitizer matrix job in CI | `.github/workflows/test.yml` | [#196](https://github.com/Urus-Foundation/Urus/pull/196) |
| Silent miscompile of tuple-of-tuple typedefs (pointer aliasing across recursive calls) fixed | `compiler/codegen.c` | [#193](https://github.com/Urus-Foundation/Urus/pull/193) |
| Hardcoded `MAX_*` caps that silently dropped declarations removed; tables grow on demand | parser, preprocess, codegen | [#188](https://github.com/Urus-Foundation/Urus/pull/188) |

### Process / IO

| Mitigation | Where | Landed |
|------------|-------|--------|
| Driver no longer creates predictable temp `.c` files (`_urus_tmp_<pid>.c`); now uses `mkstemps` / `_sopen_s` with `_O_CREAT \| _O_EXCL` | `compiler/urusc.c` | [#194](https://github.com/Urus-Foundation/Urus/pull/194) |
| `emcc` (WASM backend) is invoked via `fork`/`execvp` (POSIX) or `_spawnvp` (Windows); no shell in the loop | `compiler/urusc.c` | [#194](https://github.com/Urus-Foundation/Urus/pull/194) |
| Package manager `git clone` no longer goes through `system()`; URLs validated against a strict allowlist before being passed as `argv` | `compiler/pkg.c` | [#187](https://github.com/Urus-Foundation/Urus/pull/187) |
| Stdlib `http.urus` validator tightened to reject all bytes &lt; 0x20, DEL, and the full bash metaset | `compiler/stdlib/http.urus` | [#200](https://github.com/Urus-Foundation/Urus/pull/200) |

### Resilience

| Mitigation | Where | Landed |
|------------|-------|--------|
| Preprocessor detects circular imports and reports the cycle path instead of stack-overflowing | `compiler/preprocess.c` | [#190](https://github.com/Urus-Foundation/Urus/pull/190) |
| LSP `uri_to_path` rewritten with bounds-checked reads and propagated allocation failures (was reading past end on truncated `file:///C` URIs) | `compiler/lsp.c` | [#186](https://github.com/Urus-Foundation/Urus/pull/186) |
| Parser panic-mode recovery: a single malformed declaration no longer suppresses every following diagnostic | `compiler/parser.c` | [#198](https://github.com/Urus-Foundation/Urus/pull/198) |
| MSVC native build refused at CMake configure time (RAII via `__attribute__((cleanup))` requires GCC/Clang) — prevents silent skip of destructors | `compiler/CMakeLists.txt` | [#199](https://github.com/Urus-Foundation/Urus/pull/199) |
| Codegen byte-determinism regression test (`tests/determinism.cmake`) | `compiler/CMakeLists.txt` | [#201](https://github.com/Urus-Foundation/Urus/pull/201) |

---

## Security Design Principles

The deeper "why" behind the mitigations above. New PRs are expected to align with these.

| Principle | How URUS enforces it |
|-----------|----------------------|
| **Memory safety by default** | Automatic reference counting + runtime bounds checks on every array/string access. No pointer arithmetic in user-level URUS. |
| **No silent failures** | Compiler-internal allocators abort on OOM with a clear message rather than returning NULL; hardcoded `MAX_*` caps that used to silently truncate have been removed. |
| **No shell in the loop** | The compiler never composes a shell command with user-controlled data. External binaries (`git`, `emcc`) are invoked via `argv`-style spawn primitives, not `system()`. |
| **TOCTOU-resistant temp files** | Generated `.c` intermediates use atomic `O_EXCL` creation under an unpredictable name. |
| **Immutable by default** | URUS variables require explicit `mut` to be mutable; lowers the surface for accidental shared-mutable-state bugs. |
| **Type safety** | All types verified at compile time. No implicit numeric coercion. |
| **Network is opt-in per call** | Stdlib `http_get` / `http_post` shell out to `curl` (a deliberately narrow attack surface), and only when explicitly called. |
| **Raw escape hatch is loud** | `__emit__()` lets users inline C and bypass safety checks — but its name is intentionally ugly so it shows up in every code review. |
| **Determinism as a property** | Compiler output is byte-deterministic for the same input; this is asserted by a CTest fixture so a future change that breaks reproducible builds is caught at PR time. |

---

## Hall of Thanks

We credit security reporters in [CHANGELOG.md](./CHANGELOG.md) and in the GHSA record for each fix, unless the reporter has asked to remain anonymous. If you are the first to report a previously unknown issue, you will be listed here on the next release.

_No external reports yet — all 0.1.0 hardenings landed via internal audit. We hope to thank you here next._

---

For deeper architectural detail, see the [Security Model](./documentation/security/security-model.md) and the [stdlib audit ADR-004](./documentation/decisions/ADR-004-stdlib-audit-v0.1.0.md).
