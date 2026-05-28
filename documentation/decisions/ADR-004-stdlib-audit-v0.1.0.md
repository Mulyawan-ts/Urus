# ADR-004: Stdlib audit notes (v0.1.0 foundation)

**Status:** active, follow-ups tracked  
**Date:** 2026-05  
**Context:** Stage 2 of the v0.1.0 foundation-hardening pass touched
every other compiler subsystem (allocator, codegen, security,
parser). The stdlib was reviewed in the same pass so its known sharp
edges are written down somewhere visible instead of living in head
state. This ADR is the resulting bug-and-quirk inventory plus the
disposition for each entry.

## Scope

All seven modules under `compiler/stdlib/`:

| Module       | LOC | Notes |
|--------------|-----|-------|
| `string.urus`| 203 | string ops; uses raw `free()` (see §1) |
| `math.urus`  | 366 | numeric; no I/O, no allocations on user input |
| `fs.urus`    | 180 | filesystem; thin wrappers over libc |
| `os.urus`    | 200 | env/process; thin wrappers |
| `graph.urus` | 256 | text-mode charts; no I/O on user input |
| `json.urus`  | 329 | hand-rolled JSON parser (see §2) |
| `http.urus`  | 101 | shells out to `curl` via `popen()` (see §3) |

## 1. Allocator inconsistency (cosmetic)

Several modules call `urus_alloc()` to allocate then plain `free()`
to release (string.urus:11/24/37/48, json.urus:120, http.urus:29/48).
This is *correct* — `urus_alloc()` calls `malloc()` under the hood and
the matching `free()` is well-defined — but it makes the ownership
contract harder to follow. A wrapper `urus_free()` companion to
`urus_alloc()` / `urus_realloc()` would make every alloc/free pair
visibly use the same family.

**Disposition:** follow-up. Not in the v0.1.0 critical path; the
generated C is correct as-is.

## 2. JSON parser — silent malformed-input acceptance

`urus_json_get_int()` / `_get_float()` (json.urus:135-145) call
`strtoll` / `strtod` straight on the JSON tail. Input like
`{"k":1foo}` returns `1` instead of failing, because `strtoll` stops
at the first non-numeric character without telling us we did. Real
JSON parsers reject this; ours accepts it.

**Disposition:** follow-up. Not a security issue, but a correctness
quirk that users will eventually hit. Fix is to advance past the
number with `_json_skip_value()` and compare lengths.

## 3. `http.urus` — shells out to `curl` via `popen()` (CRITICAL design issue)

Both `urus_http_get()` and `urus_http_post()` build a shell command
string of the form

    curl -s "<url>"
    curl -s -X POST -d "<body>" "<url>"

and pass it to `popen()`. The shell parses the string before `curl`
sees it, so unescaped metacharacters in `url` or `body` would execute
arbitrary commands. The module defends against this with
`urus_http_validate_input()`, which rejects strings containing any
character we couldn't safely interpolate.

The previous allowlist was incomplete — it missed `'`, `<`, `>`,
`(`, `)`, `*`, `?`, `[`, `]`, `{`, `}`, `!`, and all ASCII control
characters except `\n` and `\r`. This stage tightens it to reject
**every** character below 0x20, DEL, and the full shell metachar set
under bash and `/bin/sh`. That closes the defense-in-depth gap.

The right fix, however, is to stop using `popen()` entirely: spawn
`curl` via `fork`/`execvp` (POSIX) or `_spawnvp` (Windows) with an
explicit `argv` array, with no shell in the loop — the same pattern
already used by `pkg.c` (PR #186) and `urusc.c` emcc invocation
(PR #194). When that lands, `urus_http_validate_input` can be
deleted entirely and `http_post` becomes usable with JSON bodies
(currently the validator rejects `"`, which all JSON contains).

**Disposition:** validator hardening landed in this PR. Argv-based
exec is a follow-up; tracked as a v0.1.0 foundation issue.

## 4. Module surface

The seven modules use ad-hoc naming and return-type conventions that
have drifted as they were added one at a time:

* `fs_file_exists`, `fs_file_size`, `fs_is_file`, `fs_is_dir`,
  `fs_delete_file`, `fs_rename_file` — `fs_` prefix is consistent.
* `string` module exposes `str_repeat`, `str_pad_left`, etc. — no
  module prefix on Urus-side names because they're accessed as
  method calls (`s.repeat(3)`); the internal C symbols are
  `urus_str_*`.
* `http_get`, `http_post` — module-prefixed.
* `os_*`, `graph_*`, `json_*` — module-prefixed.
* `math.urus` — bare names (`abs`, `sqrt`, `pow`) on Urus side;
  fine because methods on numeric types would be unergonomic.

**Disposition:** intentional. No change.

## Follow-ups

Filed against v0.1.0 stage 3 (or later):

1. Add `urus_free()` runtime helper and switch stdlib over.
2. Make JSON int/float parsing strict (reject trailing garbage).
3. Rewrite `http.urus` to use `fork+execvp(curl)` / `_spawnvp`,
   drop the validator.
4. Replace `popen()` in any other module that grows it.
