/*
 *
 * V2, ALSO KNOWN AS "VALENCIA-VILLAMER"
 * This is a scripting language.
 * Copyright (c) 2025 Cyril John Magayaga
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script.v2f>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *code = malloc(size + 1);
    fread(code, 1, size, fp);
    code[size] = '\0';
    fclose(fp);

    interpret(code);
    free(code);
    return 0;
}
