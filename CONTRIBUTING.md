# Contributing to URUS

Thank you for your interest in contributing to URUS! This guide will help you get started.

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally:
   ```bash
   git clone https://github.com/<your-username>/Urus.git
   cd Urus
   ```
3. **Build** the compiler:
   ```bash
   cd compiler
   cmake -S . -B build
   cmake --build build
   ```
4. **Run tests** to make sure everything works:
   ```bash
    cd compiler/build    # Make sure you're in the build directory
    ctest               # Run all tests
   ```

## How to Contribute

### Reporting Bugs

- Search [existing issues](https://github.com/Urus-Foundation/Urus/issues) first to avoid duplicates
- Open a new issue with:
  - URUS version (`urusc --version`)
  - OS and C compiler version
  - Minimal `.urus` file that reproduces the bug
  - Expected vs actual behavior
  - Error message (if any)

### Suggesting Features

- Open an issue with the `enhancement` label
- Describe the feature and its use case
- If possible, show example URUS syntax for the proposed feature

### Submitting Code

1. **Create a branch** from `main`:
   ```bash
   git checkout main
   git pull origin main
   git checkout -b feat/your-feature-name
   ```

2. **Make your changes** following the [coding standards](#coding-standards)

3. **Add tests** in `tests/run/`:
   - Create a `.urus` source file
   - Create a matching `.expected` file with expected output

4. **Run the test suite:**
   ```bash
    cd compiler/build    # Make sure you're in the build directory
    ctest               # Run all tests
   ```

5. **Commit** with a clear message:
   ```bash
   git commit -m "feat: short description of changes"
   ```

6. **Push** and open a Pull Request:
   ```bash
   git push -u origin feat/your-feature-name
   ```

## Coding Standards

### C Code (Compiler)

- **Standard:** C11 (`-std=c11`)
- **Indentation:** 4 spaces, no tabs (`IndentWidth: 4`, `UseTab: Never`).
- **Braces (Custom Mozilla/K&R hybrid):**
    - Opening brace on a **new line** for functions and classes (`AfterFunction: true`).
    - Opening brace on the **same line** for control statements like `if`, `for`, and `while` (`AfterControlStatement: false`).
- **Short Statements:** No single-line blocks. Functions, enums, and `if` statements must always be broken into multiple lines (`AllowShort...: false/None`).
- **Naming:** `snake_case` for everything. Public functions use a module prefix: `lexer_`.
- **Line length:** Hard limit of **80 characters** (`ColumnLimit: 80`).
- **Compile clean:** The codebase should build without warnings under `-Wall -Wextra`.

```c
/* Function: Brace on a new line */
void
lexer_advance(Lexer *l)
{
    /* Control flow: Brace on the same line */
    if (l->pos >= l->len) {
        return;
    }
    l->pos++;
}

/* Struct/Enum: Brace on the same line */
typedef struct {
    int pos;
    int len;
} Lexer;
```

### Commit Messages

Use [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>: <short description>
```

| Type | Usage |
|------|-------|
| `feat` | New feature |
| `fix` | Bug fix |
| `refactor` | Internal change, no behavior change |
| `docs` | Documentation only |
| `test` | New or updated tests |
| `chore` | Build, CI, or tooling changes |

### Adding a New Language Feature

If you're adding a new language feature, follow this checklist:

1. `compiler/include/token.h` — Add token (if new keyword/operator)
2. `compiler/src/lexer.c` — Recognize token
3. `compiler/include/ast.h` + `compiler/src/ast.c` — Add AST node type
4. `compiler/src/parser.c` — Parse new syntax
5. `compiler/src/Sema/sema.c` — Type-check
6. `compiler/src/codegen.c` — Generate C code
7. `compiler/runtime/urus_runtime.h` — Add runtime support (if needed)
8. `tests/run/` — Add test cases (`.urus` + `.expected`)
9. `SPEC.md` — Update language spec
10. `examples/` — Add example program

### Adding a Standard Library Module

Standard library modules live in `compiler/stdlib/` as `.urus` files:

1. `compiler/stdlib/your_module.urus` — Implement the module
2. Use `__emit__()` for C-level bindings when needed
3. Prefix all exported functions with the module name (e.g., `math_sin`, `json_parse`)
4. Add test cases in `tests/run/`
5. Document in `SPEC.md` under the Standard Library section

## Pull Request Guidelines

- Target the `main` branch
- Keep PRs focused — one feature or fix per PR
- Include tests for new functionality
- Update documentation if the change affects user-facing behavior
- Make sure all existing tests pass before submitting
- Describe what your PR does and why in the PR description

## Project Structure

```
compiler/
  src/              # C source files
  include/          # Header files
  runtime/          # Embedded runtime (urus_runtime.h)
  stdlib/           # Standard library modules (.urus)
tests/
  run/              # Compile, run, check output (.urus + .expected)
examples/           # Example URUS programs
documentation/      # Extended documentation
```

## Need Help?

- Read the [Documentation](./documentation/index.md)
- Check the [Language Spec](SPEC.md)
- Look at [Examples](./examples/) for syntax reference
- Open an issue if you're stuck

## License

By contributing, you agree that your contributions will be licensed under the [Apache License 2.0](./LICENSE).
