#include <stdio.h>
#include "lexer/lexer.h"

int main() {
    const char *code =
        "@nova.v1\n"
        "say \"Universal Dragon\"\n"
        "let x = 10\n"
        "qbit q = |0>\n"
        "hadamard q\n"
        "check x > 5 {\n"
        "say \"Dragon!\"\n"
        "}\n";

    int count;
    Token *tokens = tokenize(code, &count);
    for (int i=0;i<count;i++)
        printf("[%s] = '%s' (line %d)\n",
            token_name(tokens[i].type),
            tokens[i].value,
            tokens[i].line);
    return 0;
}
