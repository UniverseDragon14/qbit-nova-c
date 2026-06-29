#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("DRAG Language v0.1\n");
        printf("Usage: drag <file.drag>\n");
        return 0;
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Error: Cannot open %s\n", argv[1]);
        return 1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // say command
        if (strncmp(line, "say ", 4) == 0) {
            char *msg = line + 4;
            // Remove quotes
            if (msg[0] == '"') {
                msg++;
                msg[strlen(msg)-2] = '\0';
            }
            printf("%s\n", msg);
        }
    }
    
    fclose(f);
    return 0;
}
