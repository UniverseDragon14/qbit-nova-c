#include <stdio.h>
#include <stdlib.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/byte_vm.h"

static char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error: file not found: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *code = malloc(size + 1);
    if (!code) {
        fclose(f);
        printf("Error: memory allocation failed\n");
        return NULL;
    }

    fread(code, 1, size, f);
    code[size] = 0;
    fclose(f);

    return code;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: qnova-qbc <input.qn> <output.qbc>\n");
        return 0;
    }

    char *source = read_file(argv[1]);
    if (!source) return 1;

    int token_count = 0;
    Token *tokens = tokenize(source, &token_count);
    ASTNode *tree = parse(tokens, token_count);

    compile(tree->body);

    printf("=== COMPILED BYTECODE ===\n");
    print_bytecode();

    save_qbc(argv[2]);

    instr_count = 0;

    printf("\n=== LOADING QBC ===\n");
    if (!load_qbc(argv[2])) {
        free_ast(tree);
        free(source);
        return 1;
    }

    printf("\n=== LOADED BYTECODE ===\n");
    print_bytecode();

    printf("\n");
    run_bytecode(bytecode, instr_count);

    free_ast(tree);
    free(source);

    return 0;
}
