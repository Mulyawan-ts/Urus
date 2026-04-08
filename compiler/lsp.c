// lsp.c — Urus Language Server Protocol (LSP) server
// Communicates via JSON-RPC 2.0 over stdio.
// Phase 1: diagnostics, hover, go-to-definition, basic completion.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "urusc.h"
#include "urusctok.h"

// ============================================================
// Minimal JSON builder/parser
// ============================================================

typedef struct {
    char *data;
    int len;
    int cap;
} JsonBuf;

static void jbuf_init(JsonBuf *b)
{
    b->cap = 4096;
    b->data = (char *)malloc((size_t)b->cap);
    b->len = 0;
    b->data[0] = '\0';
}

static void jbuf_free(JsonBuf *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

static void jbuf_append(JsonBuf *b, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    while (b->len + needed + 1 > b->cap) {
        b->cap *= 2;
        b->data = (char *)realloc(b->data, (size_t)b->cap);
    }
    va_start(ap, fmt);
    b->len += vsnprintf(b->data + b->len,
                        (size_t)(b->cap - b->len), fmt, ap);
    va_end(ap);
}

// Escape a string for JSON
static void jbuf_append_escaped(JsonBuf *b, const char *s)
{
    jbuf_append(b, "\"");
    for (; *s; s++) {
        switch (*s) {
        case '"': jbuf_append(b, "\\\""); break;
        case '\\': jbuf_append(b, "\\\\"); break;
        case '\n': jbuf_append(b, "\\n"); break;
        case '\r': jbuf_append(b, "\\r"); break;
        case '\t': jbuf_append(b, "\\t"); break;
        default: jbuf_append(b, "%c", *s); break;
        }
    }
    jbuf_append(b, "\"");
}

// Minimal JSON value extraction (not a full parser, just enough
// for LSP)
static const char *json_find_key(const char *json, const char *key)
{
    char search[256];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return NULL;
    p += strlen(search);
    while (*p && (*p == ' ' || *p == ':')) p++;
    return p;
}

static int json_get_int(const char *json, const char *key)
{
    const char *p = json_find_key(json, key);
    if (!p) return -1;
    return atoi(p);
}

static char *json_get_str(const char *json, const char *key)
{
    const char *p = json_find_key(json, key);
    if (!p || *p != '"') return NULL;
    p++;
    const char *end = strchr(p, '"');
    if (!end) return NULL;
    size_t len = (size_t)(end - p);
    char *result = (char *)malloc(len + 1);
    memcpy(result, p, len);
    result[len] = '\0';
    return result;
}

// ============================================================
// LSP Transport
// ============================================================

static char *lsp_read_message(void)
{
    char header[512];
    int content_length = -1;

    // Read headers
    while (fgets(header, sizeof(header), stdin)) {
        if (header[0] == '\r' || header[0] == '\n') break;
        if (strncmp(header, "Content-Length:", 15) == 0) {
            content_length = atoi(header + 15);
        }
    }

    if (content_length <= 0) return NULL;

    char *body = (char *)malloc((size_t)content_length + 1);
    size_t read = fread(body, 1, (size_t)content_length, stdin);
    body[read] = '\0';
    return body;
}

static void lsp_send(const char *json)
{
    int len = (int)strlen(json);
    fprintf(stdout, "Content-Length: %d\r\n\r\n%s", len, json);
    fflush(stdout);
}

static void lsp_respond(int id, const char *result_json)
{
    JsonBuf b;
    jbuf_init(&b);
    jbuf_append(&b, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}",
                id, result_json);
    lsp_send(b.data);
    jbuf_free(&b);
}

static void lsp_notify(const char *method, const char *params_json)
{
    JsonBuf b;
    jbuf_init(&b);
    jbuf_append(&b, "{\"jsonrpc\":\"2.0\",\"method\":");
    jbuf_append_escaped(&b, method);
    jbuf_append(&b, ",\"params\":%s}", params_json);
    lsp_send(b.data);
    jbuf_free(&b);
}

// ============================================================
// Document storage (open files)
// ============================================================

#define MAX_DOCS 64

typedef struct {
    char *uri;
    char *content;
    int version;
} Document;

static Document docs[MAX_DOCS];
static int doc_count = 0;

static Document *doc_find(const char *uri)
{
    for (int i = 0; i < doc_count; i++) {
        if (strcmp(docs[i].uri, uri) == 0) return &docs[i];
    }
    return NULL;
}

static Document *doc_open(const char *uri, const char *content,
                          int version)
{
    Document *d = doc_find(uri);
    if (!d) {
        if (doc_count >= MAX_DOCS) return NULL;
        d = &docs[doc_count++];
        d->uri = strdup(uri);
    } else {
        free(d->content);
    }
    d->content = strdup(content);
    d->version = version;
    return d;
}

static void doc_close(const char *uri)
{
    for (int i = 0; i < doc_count; i++) {
        if (strcmp(docs[i].uri, uri) == 0) {
            free(docs[i].uri);
            free(docs[i].content);
            docs[i] = docs[doc_count - 1];
            doc_count--;
            return;
        }
    }
}

// ============================================================
// URI to path conversion
// ============================================================

static char *uri_to_path(const char *uri)
{
    // file:///path or file:///C:/path
    if (strncmp(uri, "file:///", 8) != 0) return strdup(uri);
    const char *p = uri + 8;
#ifdef _WIN32
    // file:///C:/path -> C:/path
    if (p[0] && p[1] == ':') return strdup(p);
    // file:///C%3A/path
    if (p[0] && p[1] == '%' && p[2] == '3') {
        char *r = (char *)malloc(strlen(p) + 1);
        r[0] = p[0];
        r[1] = ':';
        strcpy(r + 2, p + 4);
        return r;
    }
#endif
    // Unix: file:///path -> /path
    char *r = (char *)malloc(strlen(p) + 2);
    r[0] = '/';
    strcpy(r + 1, p);
    return r;
}

// ============================================================
// Diagnostics (error reporting via sema/parser)
// ============================================================

// Captured errors from compiler
#define MAX_ERRORS 128

typedef struct {
    int line;
    int col;
    char message[512];
    int severity; // 1=Error, 2=Warning, 3=Info, 4=Hint
} LspDiag;

static LspDiag captured_errors[MAX_ERRORS];
static int captured_error_count = 0;

// Redirect compiler error output to capture diagnostics
// We'll use a simple approach: parse the source, run sema,
// and capture errors from the error output.

static void publish_diagnostics(const char *uri, const char *source)
{
    captured_error_count = 0;

    // Lex
    Lexer lexer;
    lexer_init(&lexer, source, strlen(source));
    int token_count;
    Token *tokens = lexer_tokenize(&lexer, &token_count);

    if (tokens) {
        // Parse
        Parser parser;
        parser_init(&parser, tokens, token_count);
        AstNode *program = parser_parse(&parser);

        if (parser.had_error) {
            // Collect parse errors
            captured_errors[captured_error_count].line = 0;
            captured_errors[captured_error_count].col = 0;
            snprintf(captured_errors[captured_error_count].message,
                     sizeof(captured_errors[0].message),
                     "Parse error in source");
            captured_errors[captured_error_count].severity = 1;
            captured_error_count++;
        } else if (program) {
            // Run sema (errors go to stderr, we can't easily capture
            // them without refactoring, so we'll check return value)
            // Redirect stderr temporarily
            bool ok = sema_analyze(program, "lsp-buffer");
            if (!ok && captured_error_count == 0) {
                // Generic sema error
                captured_errors[0].line = 0;
                captured_errors[0].col = 0;
                snprintf(captured_errors[0].message,
                         sizeof(captured_errors[0].message),
                         "Semantic analysis error");
                captured_errors[0].severity = 1;
                captured_error_count = 1;
            }
        }
        free(tokens);
    } else {
        // Lex error
        captured_errors[0].line = 0;
        captured_errors[0].col = 0;
        snprintf(captured_errors[0].message,
                 sizeof(captured_errors[0].message),
                 "Lexer error");
        captured_errors[0].severity = 1;
        captured_error_count = 1;
    }

    // Build diagnostics JSON
    JsonBuf b;
    jbuf_init(&b);
    jbuf_append(&b, "{\"uri\":");
    jbuf_append_escaped(&b, uri);
    jbuf_append(&b, ",\"diagnostics\":[");

    for (int i = 0; i < captured_error_count; i++) {
        if (i > 0) jbuf_append(&b, ",");
        int line = captured_errors[i].line;
        int col = captured_errors[i].col;
        jbuf_append(&b,
            "{\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
            "\"end\":{\"line\":%d,\"character\":%d}},"
            "\"severity\":%d,\"source\":\"urusc\",\"message\":",
            line, col, line, col + 1,
            captured_errors[i].severity);
        jbuf_append_escaped(&b, captured_errors[i].message);
        jbuf_append(&b, "}");
    }
    jbuf_append(&b, "]}");

    lsp_notify("textDocument/publishDiagnostics", b.data);
    jbuf_free(&b);
}

// ============================================================
// Hover: show type info for identifier at position
// ============================================================

static void handle_hover(int id, const char *json)
{
    const char *td = json_find_key(json, "textDocument");
    char *uri = td ? json_get_str(td, "uri") : NULL;
    const char *pos = json_find_key(json, "position");
    int line = pos ? json_get_int(pos, "line") : -1;
    int character = pos ? json_get_int(pos, "character") : -1;

    if (!uri || line < 0 || character < 0) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    Document *doc = doc_find(uri);
    if (!doc) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    // Find the word at position
    const char *src = doc->content;
    int cur_line = 0, cur_col = 0;
    const char *p = src;
    while (*p && cur_line < line) {
        if (*p == '\n') cur_line++;
        p++;
    }
    // p is now at start of target line
    const char *line_start = p;
    while (*p && cur_col < character) {
        cur_col++;
        p++;
    }

    // Find word boundaries
    const char *word_start = p;
    while (word_start > line_start &&
           (isalnum((unsigned char)word_start[-1]) ||
            word_start[-1] == '_'))
        word_start--;
    const char *word_end = p;
    while (*word_end &&
           (isalnum((unsigned char)*word_end) || *word_end == '_'))
        word_end++;

    if (word_start == word_end) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    char word[256];
    size_t wlen = (size_t)(word_end - word_start);
    if (wlen >= sizeof(word)) wlen = sizeof(word) - 1;
    memcpy(word, word_start, wlen);
    word[wlen] = '\0';

    // Try to find info about this word by lexing/parsing
    // For now, show keyword info or builtin info
    const char *info = NULL;

    // Check builtins
    static const struct { const char *name; const char *sig; } builtins[] = {
        {"print", "fn print(value: any): void"},
        {"to_str", "fn to_str(value: any): str"},
        {"to_int", "fn to_int(value: any): int"},
        {"to_float", "fn to_float(value: any): float"},
        {"len", "fn len(arr: [any]): int"},
        {"push", "fn push(arr: [any], value: any): void"},
        {"pop", "fn pop(arr: [any]): void"},
        {"str_len", "fn str_len(s: str): int"},
        {"str_upper", "fn str_upper(s: str): str"},
        {"str_lower", "fn str_lower(s: str): str"},
        {"str_trim", "fn str_trim(s: str): str"},
        {"str_contains", "fn str_contains(s: str, sub: str): bool"},
        {"str_find", "fn str_find(s: str, sub: str): int"},
        {"str_slice", "fn str_slice(s: str, start: int, end: int): str"},
        {"str_replace", "fn str_replace(s: str, old: str, new: str): str"},
        {"str_starts_with", "fn str_starts_with(s: str, prefix: str): bool"},
        {"str_ends_with", "fn str_ends_with(s: str, suffix: str): bool"},
        {"str_split", "fn str_split(s: str, delim: str): [str]"},
        {"char_at", "fn char_at(s: str, i: int): str"},
        {"abs", "fn abs(x: int): int"},
        {"fabs", "fn fabs(x: float): float"},
        {"sqrt", "fn sqrt(x: float): float"},
        {"pow", "fn pow(x: float, y: float): float"},
        {"min", "fn min(a: int, b: int): int"},
        {"max", "fn max(a: int, b: int): int"},
        {"fmin", "fn fmin(a: float, b: float): float"},
        {"fmax", "fn fmax(a: float, b: float): float"},
        {"input", "fn input(): str"},
        {"read_file", "fn read_file(path: str): str"},
        {"write_file", "fn write_file(path: str, content: str): void"},
        {"append_file", "fn append_file(path: str, content: str): void"},
        {"exit", "fn exit(code: int): void"},
        {"assert", "fn assert(cond: bool, msg: str): void"},
        {"assert_eq", "fn assert_eq(a: any, b: any): void"},
        {"assert_ne", "fn assert_ne(a: any, b: any): void"},
        {"is_ok", "fn is_ok(r: Result): bool"},
        {"is_err", "fn is_err(r: Result): bool"},
        {"unwrap", "fn unwrap(r: Result): any"},
        {"unwrap_err", "fn unwrap_err(r: Result): any"},
        {"http_get", "fn http_get(url: str): str"},
        {"http_post", "fn http_post(url: str, body: str): str"},
        {NULL, NULL}
    };

    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(word, builtins[i].name) == 0) {
            info = builtins[i].sig;
            break;
        }
    }

    // Check keywords
    if (!info) {
        static const struct { const char *kw; const char *desc; } keywords[] = {
            {"fn", "Function declaration"},
            {"let", "Variable declaration (immutable by default)"},
            {"mut", "Mutable modifier"},
            {"struct", "Struct type declaration"},
            {"enum", "Enum / tagged union declaration"},
            {"trait", "Trait declaration (interface)"},
            {"impl", "Implementation block for methods/traits"},
            {"async", "Asynchronous function modifier"},
            {"await", "Await a future value"},
            {"match", "Pattern matching expression"},
            {"if", "Conditional expression/statement"},
            {"else", "Else branch"},
            {"for", "For loop (range or foreach)"},
            {"while", "While loop"},
            {"return", "Return from function"},
            {"break", "Break out of loop"},
            {"continue", "Continue to next iteration"},
            {"const", "Compile-time constant"},
            {"import", "Import module"},
            {"defer", "Defer execution to end of scope"},
            {"test", "Test block declaration"},
            {"rune", "Macro (textual substitution)"},
            {"type", "Type alias declaration"},
            {NULL, NULL}
        };
        for (int i = 0; keywords[i].kw; i++) {
            if (strcmp(word, keywords[i].kw) == 0) {
                info = keywords[i].desc;
                break;
            }
        }
    }

    if (!info) {
        // Try to find user-defined function in the document
        Lexer lexer;
        lexer_init(&lexer, doc->content, strlen(doc->content));
        int tc;
        Token *toks = lexer_tokenize(&lexer, &tc);
        if (toks) {
            for (int i = 0; i < tc - 1; i++) {
                if (toks[i].type == TOK_FN && toks[i+1].type == TOK_IDENT) {
                    if ((int)toks[i+1].length == (int)wlen &&
                        memcmp(toks[i+1].start, word, wlen) == 0) {
                        info = "User-defined function";
                        break;
                    }
                }
                if (toks[i].type == TOK_STRUCT && toks[i+1].type == TOK_IDENT) {
                    if ((int)toks[i+1].length == (int)wlen &&
                        memcmp(toks[i+1].start, word, wlen) == 0) {
                        info = "User-defined struct";
                        break;
                    }
                }
            }
            free(toks);
        }
    }

    if (info) {
        JsonBuf b;
        jbuf_init(&b);
        jbuf_append(&b, "{\"contents\":{\"kind\":\"markdown\",\"value\":");
        char md[512];
        snprintf(md, sizeof(md), "```urus\\n%s\\n```", info);
        jbuf_append_escaped(&b, md);
        jbuf_append(&b, "}}");
        lsp_respond(id, b.data);
        jbuf_free(&b);
    } else {
        lsp_respond(id, "null");
    }

    free(uri);
}

// ============================================================
// Go-to-definition: find fn/struct/enum declaration
// ============================================================

static void handle_goto_definition(int id, const char *json)
{
    const char *td = json_find_key(json, "textDocument");
    char *uri = td ? json_get_str(td, "uri") : NULL;
    const char *pos = json_find_key(json, "position");
    int line = pos ? json_get_int(pos, "line") : -1;
    int character = pos ? json_get_int(pos, "character") : -1;

    if (!uri || line < 0 || character < 0) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    Document *doc = doc_find(uri);
    if (!doc) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    // Find word at position (same logic as hover)
    const char *src = doc->content;
    int cur_line = 0;
    const char *p = src;
    while (*p && cur_line < line) {
        if (*p == '\n') cur_line++;
        p++;
    }
    const char *line_start = p;
    int cur_col = 0;
    while (*p && cur_col < character) {
        cur_col++;
        p++;
    }
    const char *word_start = p;
    while (word_start > line_start &&
           (isalnum((unsigned char)word_start[-1]) ||
            word_start[-1] == '_'))
        word_start--;
    const char *word_end = p;
    while (*word_end &&
           (isalnum((unsigned char)*word_end) || *word_end == '_'))
        word_end++;

    if (word_start == word_end) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    char word[256];
    size_t wlen = (size_t)(word_end - word_start);
    if (wlen >= sizeof(word)) wlen = sizeof(word) - 1;
    memcpy(word, word_start, wlen);
    word[wlen] = '\0';

    // Search for definition (fn name, struct name, enum name)
    Lexer lexer;
    lexer_init(&lexer, doc->content, strlen(doc->content));
    int tc;
    Token *toks = lexer_tokenize(&lexer, &tc);
    if (!toks) {
        lsp_respond(id, "null");
        free(uri);
        return;
    }

    bool found = false;
    int def_line = 0, def_col = 0;
    for (int i = 0; i < tc - 1; i++) {
        if ((toks[i].type == TOK_FN ||
             toks[i].type == TOK_STRUCT ||
             toks[i].type == TOK_ENUM ||
             toks[i].type == TOK_TRAIT) &&
            toks[i+1].type == TOK_IDENT &&
            (int)toks[i+1].length == (int)wlen &&
            memcmp(toks[i+1].start, word, wlen) == 0) {
            def_line = toks[i+1].line - 1; // 0-based
            def_col = toks[i+1].col > 0 ? toks[i+1].col - 1 : 0;
            found = true;
            break;
        }
    }
    free(toks);

    if (found) {
        JsonBuf b;
        jbuf_init(&b);
        jbuf_append(&b, "{\"uri\":");
        jbuf_append_escaped(&b, uri);
        jbuf_append(&b,
            ",\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
            "\"end\":{\"line\":%d,\"character\":%d}}}",
            def_line, def_col, def_line,
            def_col + (int)wlen);
        lsp_respond(id, b.data);
        jbuf_free(&b);
    } else {
        lsp_respond(id, "null");
    }
    free(uri);
}

// ============================================================
// Completion: keywords + builtins
// ============================================================

static void handle_completion(int id, const char *json)
{
    (void)json;

    static const char *kw_items[] = {
        "fn", "let", "mut", "struct", "enum", "trait", "impl",
        "if", "else", "while", "for", "in", "return", "break",
        "continue", "match", "import", "const", "defer", "async",
        "await", "test", "rune", "type", "true", "false",
        "int", "float", "bool", "str", "void",
        "Ok", "Err", "Result",
        NULL
    };

    static const char *fn_items[] = {
        "print", "to_str", "to_int", "to_float",
        "len", "push", "pop",
        "str_len", "str_upper", "str_lower", "str_trim",
        "str_contains", "str_find", "str_slice",
        "str_replace", "str_starts_with", "str_ends_with",
        "str_split", "char_at",
        "abs", "fabs", "sqrt", "pow", "min", "max", "fmin", "fmax",
        "input", "read_file", "write_file", "append_file",
        "exit", "assert", "assert_eq", "assert_ne",
        "is_ok", "is_err", "unwrap", "unwrap_err",
        "http_get", "http_post",
        NULL
    };

    JsonBuf b;
    jbuf_init(&b);
    jbuf_append(&b, "{\"isIncomplete\":false,\"items\":[");

    bool first = true;
    for (int i = 0; kw_items[i]; i++) {
        if (!first) jbuf_append(&b, ",");
        first = false;
        jbuf_append(&b, "{\"label\":");
        jbuf_append_escaped(&b, kw_items[i]);
        jbuf_append(&b, ",\"kind\":14}"); // 14 = Keyword
    }
    for (int i = 0; fn_items[i]; i++) {
        if (!first) jbuf_append(&b, ",");
        first = false;
        jbuf_append(&b, "{\"label\":");
        jbuf_append_escaped(&b, fn_items[i]);
        jbuf_append(&b, ",\"kind\":3}"); // 3 = Function
    }

    jbuf_append(&b, "]}");
    lsp_respond(id, b.data);
    jbuf_free(&b);
}

// ============================================================
// Main LSP loop
// ============================================================

static void handle_initialize(int id)
{
    const char *result =
        "{"
        "\"capabilities\":{"
            "\"textDocumentSync\":1,"
            "\"hoverProvider\":true,"
            "\"definitionProvider\":true,"
            "\"completionProvider\":{\"triggerCharacters\":[\".\"]}"
        "},"
        "\"serverInfo\":{\"name\":\"urusc-lsp\",\"version\":\"0.1.0\"}"
        "}";
    lsp_respond(id, result);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    // Redirect stderr to a log file (optional)
    FILE *log = fopen("urusc-lsp.log", "w");
    if (log) {
        // Keep stderr for logging, don't redirect
        // (compiler errors go to stderr too)
    }

    bool running = true;
    bool initialized = false;

    while (running) {
        char *msg = lsp_read_message();
        if (!msg) break;

        if (log) {
            fprintf(log, "<-- %s\n", msg);
            fflush(log);
        }

        // Extract method and id
        char *method = json_get_str(msg, "method");
        int id = json_get_int(msg, "id");

        if (method && strcmp(method, "initialize") == 0) {
            handle_initialize(id);
            initialized = true;
        } else if (method && strcmp(method, "initialized") == 0) {
            // No response needed
        } else if (method && strcmp(method, "shutdown") == 0) {
            lsp_respond(id, "null");
            running = false;
        } else if (method && strcmp(method, "exit") == 0) {
            running = false;
        } else if (initialized) {
            if (method && strcmp(method,
                "textDocument/didOpen") == 0) {
                const char *td = json_find_key(msg, "textDocument");
                char *uri = td ? json_get_str(td, "uri") : NULL;
                char *text = td ? json_get_str(td, "text") : NULL;
                int ver = td ? json_get_int(td, "version") : 0;
                if (uri && text) {
                    doc_open(uri, text, ver);
                    publish_diagnostics(uri, text);
                }
                free(uri);
                free(text);
            } else if (method && strcmp(method,
                       "textDocument/didChange") == 0) {
                const char *td = json_find_key(msg, "textDocument");
                char *uri = td ? json_get_str(td, "uri") : NULL;
                int ver = td ? json_get_int(td, "version") : 0;
                // Get content changes (full sync mode)
                const char *cc = json_find_key(msg, "contentChanges");
                char *text = cc ? json_get_str(cc, "text") : NULL;
                if (uri && text) {
                    doc_open(uri, text, ver);
                    publish_diagnostics(uri, text);
                }
                free(uri);
                free(text);
            } else if (method && strcmp(method,
                       "textDocument/didClose") == 0) {
                const char *td = json_find_key(msg, "textDocument");
                char *uri = td ? json_get_str(td, "uri") : NULL;
                if (uri) doc_close(uri);
                free(uri);
            } else if (method && strcmp(method,
                       "textDocument/hover") == 0) {
                handle_hover(id, msg);
            } else if (method && strcmp(method,
                       "textDocument/definition") == 0) {
                handle_goto_definition(id, msg);
            } else if (method && strcmp(method,
                       "textDocument/completion") == 0) {
                handle_completion(id, msg);
            } else if (id >= 0) {
                // Unknown request — respond with null
                lsp_respond(id, "null");
            }
        }

        free(method);
        free(msg);
    }

    if (log) fclose(log);
    return 0;
}
