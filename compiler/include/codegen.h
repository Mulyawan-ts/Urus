#ifndef URUS_CODEGEN_H
#define URUS_CODEGEN_H

#include "ast.h"
#include <stdbool.h>

typedef struct {
    char *c_name;
    AstType *type;
} CGSym;

typedef struct CGScope {
    CGSym syms[128]; // TODO: Use dynamic memory than static 128 (other reference: raii_register()@codegen.c)
    int count;
    bool is_loop;
    struct CGScope *parent;
} CGScope;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
    int indent;
    int tmp_counter;
    CGScope *current_scope;
} CodeBuf;

void codegen_init(CodeBuf *buf);
void codegen_free(CodeBuf *buf);
void codegen_generate(CodeBuf *buf, AstNode *program);

#endif
