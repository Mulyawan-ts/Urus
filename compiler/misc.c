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

char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t len = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = xmalloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    if (out_len)
        *out_len = len;
    return buf;
}

// --  Memory Management  --
//
// All compiler-internal allocations funnel through these wrappers. They
// abort on out-of-memory rather than returning NULL, so callers must
// never null-check the result. If you find yourself writing
// `if (!xmalloc(...))`, the check is dead code — delete it.

void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed; out of memory.\n");
        abort();
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size)
{
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed; out of memory.\n");
        abort();
    }
    return ptr;
}

char *xstrdup(const char *s)
{
    if (!s) {
        // strdup(NULL) is UB; surface the bug instead of silently
        // returning NULL like some libc implementations do.
        fprintf(stderr, "xstrdup called with NULL argument.\n");
        abort();
    }
    size_t len = strlen(s);
    char *out = (char *)xmalloc(len + 1);
    memcpy(out, s, len + 1);
    return out;
}

void *__xrealloc(void **ptr, size_t size)
{
    void *new_ptr = realloc(*ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Memory re-allocation failed; out of memory.\n");
        abort();
    }
    *ptr = new_ptr;
    return new_ptr;
}

void __xfree(void **ptr)
{
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}
