#include <stdio.h>
#include "lexer/lexer.h"
#include "parser/parser.h"

int main() {
    const char *code =
        "@nova.v1\n"
        "say \"Universal Dragon\"\n"
        "let x = 10\n"
        "qbit q = |0>\n"
        "hadamard q\n"
        "measure q\n"
        "check x > 5 {\n"
        "say \"Dragon!\"\n"
        "}\n";

    int count;
    Token *tokens = tokenize(code, &count);
    ASTNode *tree = parse(tokens, count);
    printf("=== QBIT NOVA AST ===\n");
    print_ast(tree->body, 0);
    free_ast(tree);
    return 0;
}
