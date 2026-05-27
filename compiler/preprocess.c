/*
 * Copyright 2026 Urus Foundation (https://github.com/Urus-Foundation)
 *
 * This file is part of the Urus Programming Language.
 * For more about this language check at
 *
 *    https://github.com/Urus-Foundation/Urus
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "urusc.h"

// --- Import resolution ---

// Track imported files to dedupe imports. This list both serves as a
// "seen" set (so each file is processed at most once) and grows on
// demand, replacing the previous hardcoded MAX_IMPORTS=64 cap.
//
// Entries are not owned by this table — some come from the caller
// (base_file) and some are heap-allocated by resolve_*_path and freed
// elsewhere — so this table itself never frees its entries.
static char **imported_files = NULL;
static int import_count = 0;
static int import_cap = 0;

static void imports_reserve(int needed)
{
    if (needed <= import_cap)
        return;
    int new_cap = import_cap == 0 ? 16 : import_cap;
    while (new_cap < needed)
        new_cap *= 2;
    imported_files = xrealloc(imported_files,
                              sizeof(*imported_files) * (size_t)new_cap);
    import_cap = new_cap;
}

// import_chain is a stack of files whose preprocess_imports() call is
// currently *in progress* (i.e. has not yet returned). If we encounter
// an import whose resolved path is already on this stack, we have a
// true import cycle (A imports B which imports A), which previously
// would have either infinite-recursed or just confused the user with a
// late parse error. We now refuse the import with a clear chain dump.
//
// The chain grows on demand alongside imported_files.
static char **import_chain = NULL;
static int import_chain_len = 0;
static int import_chain_cap = 0;

static void import_chain_reserve(int needed)
{
    if (needed <= import_chain_cap)
        return;
    int new_cap = import_chain_cap == 0 ? 16 : import_chain_cap;
    while (new_cap < needed)
        new_cap *= 2;
    import_chain = xrealloc(import_chain,
                            sizeof(*import_chain) * (size_t)new_cap);
    import_chain_cap = new_cap;
}

static bool already_imported(const char *path)
{
    for (int i = 0; i < import_count; i++) {
        if (strcmp(path, imported_files[i]) == 0)
            return true;
    }
    return false;
}

// Is `path` currently being processed further up the call stack?
static int import_chain_index(const char *path)
{
    for (int i = 0; i < import_chain_len; i++) {
        if (strcmp(path, import_chain[i]) == 0)
            return i;
    }
    return -1;
}

// Pretty-print the chain of files starting at `start_idx` to stderr,
// ending with the offending import that closed the cycle.
static void report_import_cycle(int start_idx, const char *closing_path)
{
    fprintf(stderr, "Error: circular import detected:\n");
    for (int i = start_idx; i < import_chain_len; i++) {
        fprintf(stderr, "  %s\n      imports\n", import_chain[i]);
    }
    fprintf(stderr, "  %s   <-- closes the cycle\n", closing_path);
}

// TODO: Add check if path is same with URUSCPATH even though is using "../"
static bool is_path_allowed(char *path)
{
    char *p = path;
    while (*p) {
        while (*p == '/' || *p == '\\')
            p++;
        const char *start = p;
        while (*p && *p != '/' && *p != '\\')
            p++;
        size_t len = (size_t)(p - start);
        if (len == 2 && start[0] == '.' && start[1] == '.')
            return false;
    }
    return true;
}

void get_local_libpath(char *out, size_t size)
{

#if defined(_WIN32)
    const char *prefix = getenv("URUSCPATH");
    if (prefix) {
        snprintf(out, size, "%s", prefix);
    } else {
        const char *drive = getenv("SystemDrive");
        if (!drive)
            drive = "C:";
        snprintf(out, size, "%s\\Program Files\\Urusc\\Lib", drive);
    }

#elif defined(__ANDROID__)
    // Android / Termux
    const char *termux = getenv("PREFIX");
    if (termux) {
        snprintf(out, size, "%s/lib/urusc", termux);
    } else {
        // Android native (not Termux)
        snprintf(out, size, "/system/lib/urusc");
    }

#elif defined(__linux__)
    const char *prefix = getenv("URUSCPATH");
    if (prefix) {
        snprintf(out, size, "%s", prefix);
    } else {
        char local_path[512];
        snprintf(local_path, sizeof(local_path), "/usr/local/lib/urusc");

        // if local exists = the user is building the compiler itself
        FILE *f = fopen(local_path, "r");
        if (f) {
            fclose(f);
            snprintf(out, size, "%s", local_path);
        } else {
            snprintf(out, size, "/usr/lib/urusc");
        }
    }

#else
    // fallback POSIX generic
    snprintf(out, size, "/usr/local/lib/urusc");
#endif
}

// resolve library import path
static char *resolve_stdlib_path(const char *module_name)
{
    char urus_path[PATH_MAX];
    get_local_libpath(urus_path, sizeof(urus_path));

    size_t base_len = strlen(urus_path);
    size_t name_len = strlen(module_name);
    // +1 for sep, +5 for ".urus", +1 for '\0'
    size_t total = base_len + 1 + name_len + 5 + 1;
    char *full = xmalloc(total);
    snprintf(full, total, "%s%c%s.urus", urus_path, URUSC_PATHSEP, module_name);

    return full;
}

// resolve relative import path
static char *resolve_import_path(const char *base_file, const char *import_path)
{
    // Find last / or backslash in base_file
    const char *last_sep = NULL;
    for (const char *p = base_file; *p; p++) {
        if (*p == '/' || *p == '\\')
            last_sep = p;
    }

    if (!last_sep) {
        return xstrdup(import_path);
    }

    size_t dir_len = (size_t)(last_sep - base_file + 1);
    size_t imp_len = strlen(import_path);
    char *full = xmalloc(dir_len + imp_len + 1);
    memcpy(full, base_file, dir_len);
    memcpy(full + dir_len, import_path, imp_len);
    full[dir_len + imp_len] = '\0';
    return full;
}

bool preprocess_imports(AstNode *program, const char *base_file)
{
    // Mark base file as imported (so we don't process it twice) and push
    // it onto the in-progress chain so that any import below us that
    // resolves back to base_file is reported as a cycle rather than
    // silently dropped or infinite-recursed. We cast away const for
    // storage; we never write through these pointers.
    if (!already_imported(base_file)) {
        imports_reserve(import_count + 1);
        imported_files[import_count++] = (char *)base_file;
    }
    import_chain_reserve(import_chain_len + 1);
    import_chain[import_chain_len++] = (char *)base_file;

    for (int i = 0; i < program->as.program.decl_count; i++) {
        AstNode *d = program->as.program.decls[i];
        if (d->kind != NODE_IMPORT)
            continue;

        char *path;

        if (d->as.import_decl.is_stdlib) {
            path = resolve_stdlib_path(d->as.import_decl.path);
        } else {
            path = resolve_import_path(base_file, d->as.import_decl.path);
            if (!is_path_allowed(path)) {
                fprintf(stderr,
                        "Error: import path '%s' resolves outside allowed "
                        "directories\n",
                        d->as.import_decl.path);
                xfree(path);
                import_chain_len--;
                return false;
            }
        }

        // Cycle check: is `path` already being processed up the call
        // stack? If so, refuse with a clear chain dump instead of
        // recursing forever.
        int cyc = import_chain_index(path);
        if (cyc >= 0) {
            report_import_cycle(cyc, path);
            xfree(path);
            import_chain_len--;
            return false;
        }

        if (already_imported(path)) {
            xfree(path);
            continue;
        }

        imports_reserve(import_count + 1);
        imported_files[import_count++] = path;

        size_t len;
        char *source = read_file(path, &len);
        if (!source) {
            fprintf(stderr, "Error: cannot import '%s'\n",
                    d->as.import_decl.path);
            if (d->as.import_decl.is_stdlib)
                fprintf(stderr, "Tip: make sure you've installed urus stdlib "
                                "correctly in your environment\n");
            import_chain_len--;
            return false;
        }

        Lexer lexer;
        lexer_init(&lexer, source, len);
        int token_count;
        Token *tokens = lexer_tokenize(&lexer, &token_count);
        if (!tokens) {
            xfree(source);
            import_chain_len--;
            return false;
        }

        Parser parser;
        parser.filename = path;
        parser_init(&parser, tokens, token_count);
        AstNode *imported = parser_parse(&parser);

        if (parser.had_error) {
            fprintf(stderr, "Error parsing imported file '%s'\n", path);
            ast_free(imported);
            xfree(tokens);
            xfree(source);
            import_chain_len--;
            return false;
        }

        // Recursively process imports in the imported file. The recursive
        // call will push/pop its own entry on import_chain; we keep ours
        // pushed for the duration of this loop iteration so a sibling
        // import that names base_file is still detected as a cycle.
        if (!preprocess_imports(imported, path)) {
            ast_free(imported);
            xfree(tokens);
            xfree(source);
            import_chain_len--;
            return false;
        }

        // Merge imported declarations into program (insert before current
        // position)
        int new_count = program->as.program.decl_count +
                        imported->as.program.decl_count - 1;
        AstNode **new_decls =
            xmalloc(sizeof(AstNode *) * (size_t)(new_count + 1));

        int pos = 0;
        // Copy declarations before the import statement
        for (int j = 0; j < i; j++) {
            new_decls[pos++] = program->as.program.decls[j];
        }
        // Insert imported declarations (skip imports from imported file)
        for (int j = 0; j < imported->as.program.decl_count; j++) {
            if (imported->as.program.decls[j]->kind != NODE_IMPORT) {
                imported->as.program.decls[j]->is_imported = true;
                new_decls[pos++] = imported->as.program.decls[j];
                imported->as.program.decls[j] = NULL; // transfer ownership
            }
        }
        // Skip the import statement itself
        // Copy declarations after the import
        for (int j = i + 1; j < program->as.program.decl_count; j++) {
            new_decls[pos++] = program->as.program.decls[j];
        }

        xfree(program->as.program.decls);
        program->as.program.decls = new_decls;
        program->as.program.decl_count = pos;

        // Don't free imported->decls since we transferred ownership
        xfree(imported->as.program.decls);
        imported->as.program.decls = NULL;
        imported->as.program.decl_count = 0;
        ast_free(imported);
        xfree(tokens);
        // Note: source memory is borrowed by tokens, don't free yet

        // Re-scan from beginning since we modified the array
        i = -1;
    }
    import_chain_len--;
    return true;
}
