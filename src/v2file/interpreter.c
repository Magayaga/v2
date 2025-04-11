/*
 *
 * V2, ALSO KNOWN AS "VALENCIA-VILLAMER"
 * This is a scripting language.
 * Copyright (c) 2025 Cyril John Magayaga
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"

void trim(char *str) {
    int len = strlen(str);
    while(len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = '\0';
    }
}

void parse_line(char *line) {
    trim(line);

    if (strncmp(line, "createfile.system(", 18) == 0) {
        char *arg = strtok(line + 18, ")");
        if (arg) {
            trim(arg);
            FILE *f = fopen(arg, "w");
            if (f) {
                fclose(f);
                printf("File created: %s\n", arg);
            } else {
                fprintf(stderr, "Failed to create file: %s\n", arg);
            }
        }
    } else if (strncmp(line, "os.system(", 10) == 0) {
        char *arg = strtok(line + 10, ")");
        if (arg) {
            trim(arg);
            system(arg);
        }
    } else if (strncmp(line, "print.system(", 13) == 0) {
        char *arg = strtok(line + 13, ")");
        if (arg) {
            trim(arg);
            printf("%s\n", arg);
        }
    } else if (strncmp(line, "error.system(", 13) == 0) {
        char *arg = strtok(line + 13, ")");
        if (arg) {
            trim(arg);
            fprintf(stderr, "%s\n", arg);
        }
    }
}

void interpret(const char *code) {
    char *copy = strdup(code);
    char *line = strtok(copy, "\n");

    while (line) {
        parse_line(line);
        line = strtok(NULL, "\n");
    }

    free(copy);
}
