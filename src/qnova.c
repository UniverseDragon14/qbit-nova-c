#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/byte_vm.h"
#include "vm/state2_vm.h"

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

static int wants_state2(const char *source) {
    return strstr(source, "@quantum.state2") != NULL;
}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    if (argc < 2) {
        printf("Usage: qnova <file.qn>\n");
        return 0;
    }

    char *source = read_file(argv[1]);
    if (!source) return 1;

    int use_state2 = wants_state2(source);

    int token_count = 0;
    Token *tokens = tokenize(source, &token_count);
    ASTNode *tree = parse(tokens, token_count);

    compile(tree->body);

    printf("=== QBIT NOVA UNIFIED RUNNER ===\n");
    printf("[RUNNER] mode: %s\n\n", use_state2 ? "STATE2" : "BYTE_VM");

    printf("=== QBIT NOVA BYTECODE ===\n");
    print_bytecode();

    if (use_state2) {
        run_state2_bytecode(bytecode, instr_count);
    } else {
        printf("\n");
        run_bytecode(bytecode, instr_count);
    }

    free_ast(tree);
    free(source);

    return 0;
}
