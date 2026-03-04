# Architecture

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────┐
│                     URUS Compiler                         │
│                                                          │
│  .urus file                                              │
│      │                                                   │
│      ▼                                                   │
│  ┌────────┐   tokens   ┌────────┐   AST   ┌──────┐     │
│  │ Lexer  │──────────▶│ Parser │────────▶│ Sema │     │
│  │        │            │        │          │      │     │
│  └────────┘            └────────┘          └──┬───┘     │
│   lexer.c               parser.c              │          │
│   token.h                ast.h/c              │          │
│                                               ▼          │
│                                          ┌─────────┐    │
│                                          │ Codegen │    │
│                                          │         │    │
│                                          └────┬────┘    │
│                                               │          │
│                                          .c output       │
│                                               │          │
└───────────────────────────────────────────────┼──────────┘
                                                │
                                                ▼
                                           ┌─────────┐
                                           │   GCC   │
                                           └────┬────┘
                                                │
                                           native binary
```

## System Flow

### 1. Lexing (Tokenization)
- Input: source code string
- Output: array of tokens
- File: `lexer.c`, `token.h`
- Recognizes: keywords, identifiers, literals, operators, f-strings

### 2. Parsing
- Input: token stream
- Output: Abstract Syntax Tree (AST)
- File: `parser.c`, `ast.h`, `ast.c`
- Recursive descent parser
- Pratt parsing for operator precedence
- Desugaring f-strings into `to_str()` + concat

### 3. Semantic Analysis (Sema)
- Input: AST
- Output: AST with resolved types
- File: `sema.c`
- Two-pass: (1) register declarations, (2) type-check bodies
- Validates: types, scopes, mutability, break/continue placement

### 4. Code Generation
- Input: type-checked AST
- Output: C source code string
- File: `codegen.c`
- Generates standard C11 (no GCC extensions)
- Embeds `urus_runtime.h` via `#include`

### 5. Compilation
- Input: generated C file
- Output: native binary
- File: `main.c` (orchestrator)
- Invokes GCC with `-std=c11 -lm`

## Folder Structure

```
D:\Urus\
├── compiler/
│   ├── src/                 # Source files (.c)
│   │   ├── main.c           # CLI entry point, import resolution
│   │   ├── lexer.c          # Tokenizer
│   │   ├── parser.c         # Recursive descent parser
│   │   ├── ast.c            # AST utilities (print, clone, free)
│   │   ├── sema.c           # Semantic analysis & type checking
│   │   ├── codegen.c        # C code generation
│   │   └── util.c           # String/memory helpers
│   │
│   ├── include/             # Header files (.h)
│   │   ├── token.h          # Token types enum
│   │   ├── lexer.h          # Lexer API
│   │   ├── parser.h         # Parser API
│   │   ├── ast.h            # AST node types, AstType definitions
│   │   ├── sema.h           # Sema API
│   │   ├── codegen.h        # Codegen API, CodeBuf struct
│   │   ├── util.h           # Utility API
│   │   └── urus_runtime.h   # Header-only runtime library
│   │
│   ├── Makefile             # Linux/macOS build
│   └── build.bat            # Windows build
│
├── examples/                # Example URUS programs
├── tests/                   # Test suite
├── documentation/           # Project documentation
├── SPEC.md                  # Language specification
├── README.md                # Project README
├── CHANGELOG.md             # Version history
├── CLAUDE.md                # AI development guide
└── LICENSE                  # Apache 2.0
```

## Inter-Module Dependencies

```
main.c
  ├── lexer.h    (tokenize source)
  ├── parser.h   (parse tokens → AST)
  ├── sema.h     (type-check AST)
  ├── codegen.h  (generate C from AST)
  ├── ast.h      (AST data structures)
  └── util.h     (string helpers)

lexer.c
  ├── lexer.h
  └── token.h    (token type enum)

parser.c
  ├── parser.h
  ├── lexer.h    (sub-lexer for f-strings)
  ├── ast.h      (create AST nodes)
  └── token.h

sema.c
  ├── sema.h
  └── ast.h      (traverse/modify AST)

codegen.c
  ├── codegen.h
  └── ast.h      (traverse AST)

ast.c
  └── ast.h
```

### Data Flow

```
Source (.urus) → Token[] → AstNode* (tree) → AstNode* (typed) → char* (C code) → binary
                 Lexer      Parser             Sema               Codegen         GCC
```
