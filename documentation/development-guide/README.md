# Development Guide

## Coding Standard

### C Language (Compiler)

- **Standard:** C11 (`-std=c11`)
- **Naming:** `snake_case` for all functions and variables
- **Prefix:** All public APIs use a module prefix: `lexer_`, `parser_`, `sema_`, `codegen_`, `ast_`
- **Runtime prefix:** `urus_` for all runtime functions
- **Indentation:** 4 spaces (not tabs)
- **Max line length:** ~100 characters (soft limit)
- **Braces:** K&R style (opening brace on the same line)

```c
// Correct style example
void lexer_next(Lexer *l) {
    if (l->pos >= l->len) {
        return;
    }
    // ...
}
```

### Memory Rules

- All heap allocations must have a matching free
- String literals in AST: duplicate with `strdup()`, free in `ast_free()`
- Runtime strings: ref-counted via `urus_str_retain/release`
- Static buffers: use round-robin to avoid clobber (see `ast_type_str`)

### Error Handling in the Compiler

- Fatal errors: print to stderr, then `exit(1)`
- Error format: `filename:line:col: error: message`
- No error recovery — compiler stops at the first error

## Branch Strategy

```
main          ← stable, released versions
  │
  ├── dev     ← development branch
  │   │
  │   ├── feature/enums
  │   ├── feature/imports
  │   └── fix/array-bounds
  │
  └── release/v1.1.0
```

| Branch | Purpose |
|--------|---------|
| `main` | Stable release, always compilable |
| `dev` | Integration branch for new features |
| `feature/*` | One feature per branch |
| `fix/*` | Bug fixes |
| `release/*` | Release preparation |

## How to Create a Pull Request

1. **Create a branch** from `dev`:
   ```bash
   git checkout dev
   git pull
   git checkout -b feature/feature-name
   ```

2. **Develop & commit:**
   ```bash
   git add src/file.c include/file.h
   git commit -m "Add: short description of changes"
   ```

3. **Test:**
   ```bash
   cd compiler && make clean && make
   cd ../tests && bash run_tests.sh
   ```

4. **Push & create PR:**
   ```bash
   git push -u origin feature/feature-name
   # Create PR via GitHub/GitLab UI
   ```

### Commit Message Convention

```
<type>: <short description>

<optional body>
```

| Type | Description |
|------|-------------|
| `Add` | New feature |
| `Fix` | Bug fix |
| `Update` | Changes to an existing feature |
| `Refactor` | Internal changes without behavior change |
| `Docs` | Documentation |
| `Test` | New tests or test updates |

## Testing Guideline

### Test Structure

```
tests/
├── valid/       # Programs that must compile without errors
├── invalid/     # Programs that must produce compile errors
├── run/         # Programs that are compiled, run, and output checked
│   ├── test.urus
│   └── test.expected
├── run_tests.sh   # Test runner (Linux/macOS)
└── run_tests.bat  # Test runner (Windows)
```

### Test Types

| Type | Folder | Pass Condition |
|------|--------|----------------|
| Valid | `valid/` | `urusc --emit-c file.urus` exits 0 |
| Invalid | `invalid/` | `urusc --emit-c file.urus` exits non-0 |
| Run | `run/` | Program output matches `.expected` exactly |

### Adding New Tests

**Valid test:**
```bash
# Create a .urus file in tests/valid/
echo 'fn main(): void { print("ok"); }' > tests/valid/new_test.urus
```

**Invalid test:**
```bash
# Create a .urus file that should produce an error
echo 'fn main(): void { let x: int = "string"; }' > tests/invalid/type_error.urus
```

**Run test:**
```bash
# 1. Create the program
cat > tests/run/new_test.urus << 'EOF'
fn main(): void {
    print("expected output");
}
EOF

# 2. Create the expected output
echo "expected output" > tests/run/new_test.expected
```

### Running Tests

```bash
cd tests/

# Linux/macOS
bash run_tests.sh ../compiler/urusc

# Windows
run_tests.bat ..\compiler\urusc.exe
```

## How to Add a New Feature

### Checklist

1. **Token** — Add new token in `token.h` if a new keyword/operator is needed
2. **Lexer** — Recognize the new token in `lexer.c`
3. **AST** — Add node type in `ast.h`, constructor in `ast.c`
4. **Parser** — Parse new syntax in `parser.c`
5. **Sema** — Type-check in `sema.c`
6. **Codegen** — Generate C code in `codegen.c`
7. **Runtime** — Add runtime support in `urus_runtime.h` if needed
8. **Test** — Add tests in `tests/`
9. **Docs** — Update SPEC.md and documentation/
10. **Example** — Add example in `examples/`

### Example: Adding the `**` (Power) Operator

```
1. token.h    → TOK_POWER
2. lexer.c    → recognize "**"
3. ast.h      → (reuse NODE_BINARY, op = "**")
4. parser.c   → add to precedence table
5. sema.c     → type-check: numeric operands
6. codegen.c  → emit pow(a, b)
7. runtime    → (pow from math.h, already available)
8. tests/     → valid/power.urus, run/power.urus + .expected
9. SPEC.md    → add ** to operators
```
