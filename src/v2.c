/*
 *
 * V2, ALSO KNOWN AS "VALENCIA-VILLAMER"
 * This is a configuration as code language with powerful tooling.
 * Copyright (c) 2024-2025 Cyril John Magayaga
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the basic data structure for a configuration item
typedef struct ConfigItem {
    char *key;
    char *value;
    struct ConfigItem *next;
    struct ConfigItem *child;
} ConfigItem;

// Function to create a new ConfigItem
ConfigItem *createConfigItem(const char *key, const char *value) {
    ConfigItem *item = (ConfigItem *)malloc(sizeof(ConfigItem));
    if (!item) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    item->key = strdup(key);
    if (!item->key) {
        free(item);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    if (value) {
        item->value = strdup(value);
        if (!item->value) {
            free(item->key);
            free(item);
            fprintf(stderr, "Memory allocation failed\n");
            return NULL;
        }
    }
    
    else {
        item->value = NULL;
    }
    item->next = NULL;
    item->child = NULL;
    return item;
}

// Function to add a child to a ConfigItem
void addChild(ConfigItem *parent, ConfigItem *child) {
    if (!parent->child) {
        parent->child = child;
    }
    
    else {
        ConfigItem *sibling = parent->child;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = child;
    }
}

// Function to free a ConfigItem
void freeConfigItem(ConfigItem *item) {
    if (item) {
        if (item->key) free(item->key);
        if (item->value) free(item->value);
        if (item->child) freeConfigItem(item->child);
        if (item->next) freeConfigItem(item->next);
        free(item);
    }
}

// Function to parse a .v2 configuration file
ConfigItem *parseV2Config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return NULL;
    }

    ConfigItem *root = createConfigItem("root", NULL);
    if (!root) {
        fclose(file);
        return NULL;
    }

    ConfigItem *current = root;
    ConfigItem *stack[128];
    int stackIndex = 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        char key[128], value[128];
        if (sscanf(line, " %127[^=]=%127[^\n]", key, value) == 2) {
            ConfigItem *item = createConfigItem(key, value);
            if (!item) {
                fclose(file);
                freeConfigItem(root);
                return NULL;
            }
            addChild(current, item);
        }
        
        else if (sscanf(line, " %127[^{] {", key) == 1) {
            ConfigItem *item = createConfigItem(key, NULL);
            if (!item) {
                fclose(file);
                freeConfigItem(root);
                return NULL;
            }
            addChild(current, item);
            stack[stackIndex++] = current;
            current = item;
        }
        
        else if (strstr(line, "}")) {
            if (stackIndex > 0) {
                current = stack[--stackIndex];
            }
            
            else {
                fprintf(stderr, "Syntax error: Unmatched closing brace\n");
                fclose(file);
                freeConfigItem(root);
                return NULL;
            }
        }
    }

    fclose(file);
    return root;
}

// Function to escape JSON strings
// Escape JSON strings properly
void escapeJSONString(const char *input, char *output) {
    while (*input) {
        switch (*input) {
            case '"': *output++ = '\\'; *output++ = '"'; break;
            case '\\': *output++ = '\\'; *output++ = '\\'; break;
            case '\b': *output++ = '\\'; *output++ = 'b'; break;
            case '\f': *output++ = '\\'; *output++ = 'f'; break;
            case '\n': *output++ = '\\'; *output++ = 'n'; break;
            case '\r': *output++ = '\\'; *output++ = 'r'; break;
            case '\t': *output++ = '\\'; *output++ = 't'; break;
            default: *output++ = *input; break;
        }
        input++;
    }
    *output = '\0';
}

// Count how many times a key appears in siblings
int countSameKey(ConfigItem *start, const char *key) {
    int count = 0;
    while (start) {
        if (strcmp(start->key, key) == 0) count++;
        start = start->next;
    }
    return count;
}

// JSON serialization (children only)
void serializeJSON(ConfigItem *item, FILE *file) {
    if (!item || !item->child) return;

    ConfigItem *child = item->child;
    fprintf(file, "{");
    int first = 1;

    while (child) {
        int keyCount = countSameKey(child, child->key);
        if (!first) fprintf(file, ",");
        first = 0;

        char escapedKey[256];
        escapeJSONString(child->key, escapedKey);

        if (keyCount > 1) {
            fprintf(file, "\"%s\": [", escapedKey);
            int arrayFirst = 1;

            // Write each item with the same key
            while (child && strcmp(child->key, escapedKey) == 0) {
                if (!arrayFirst) fprintf(file, ",");
                arrayFirst = 0;

                if (child->child) {
                    serializeJSON(child, file);
                } else if (child->value) {
                    char escapedValue[256];
                    escapeJSONString(child->value, escapedValue);
                    fprintf(file, "\"%s\"", escapedValue);
                } else {
                    fprintf(file, "null");
                }

                child = child->next;
            }
            fprintf(file, "]");
            continue; // Skip increment below
        }

        // Single value
        fprintf(file, "\"%s\": ", escapedKey);
        if (child->child) {
            serializeJSON(child, file);
        } else if (child->value) {
            char escapedValue[256];
            escapeJSONString(child->value, escapedValue);
            fprintf(file, "\"%s\"", escapedValue);
        } else {
            fprintf(file, "null");
        }

        child = child->next;
    }
    fprintf(file, "}");
}

// JSON serialization including top-level object
void serializeJSONRoot(ConfigItem *item, FILE *file) {
    if (!item) return;
    serializeJSON(item, file);
}

// Function to serialize a ConfigItem to YAML
void serializeYAML(ConfigItem *item, FILE *file, int indent) {
    if (!item) return;

    ConfigItem *child = item->child;
    while (child) {
        for (int i = 0; i < indent; ++i) fprintf(file, "  ");
        if (child->value) {
            fprintf(file, "%s: %s\n", child->key, child->value);
        }
        
        else {
            fprintf(file, "%s:\n", child->key);
            serializeYAML(child, file, indent + 1);
        }
        child = child->next;
    }
}

// Function to remove file extension and add new extension
void changeFileExtension(const char *input, char *output, const char *newExt) {
    strcpy(output, input);
    char *dot = strrchr(output, '.');
    if (dot) {
        strcpy(dot, newExt);
    }
    
    else {
        strcat(output, newExt);
    }
}

// Function to interpret the configuration file
void interpretConfig(ConfigItem *item, int indent) {
    if (!item) return;

    ConfigItem *child = item->child;
    while (child) {
        for (int i = 0; i < indent; ++i) printf("  ");
        if (child->value) {
            printf("%s = %s\n", child->key, child->value);
        }
        
        else {
            printf("%s:\n", child->key);
            interpretConfig(child, indent + 1);
        }
        child = child->next;
    }
}

// Main function to process multiple .v2 files
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [options] [filename] ...\n", argv[0]);
        return 1;
    }

    int transpileJSON = 0;
    int transpileYAML = 0;
    int loadAndInterpret = 0;
    char *loadFilename = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("%s [v1.0.0]\n", argv[0]);
            return 0;
        }
        
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options] [filename]\n\n", argv[0]);
            printf("Options\n");
            printf("   -h, --help                 Display this information.\n");
            printf("   -v, --version              Display compiler version information.\n");
            printf("   --author                   Display the author information.\n");
            printf("   --transpiler::json         Transpile to JSON format.\n");
            printf("   --transpiler::yaml         Transpile to YAML format.\n");
            printf("   --load [filename]          Load and interpret the .v2 file.\n");
            printf("\nFor bug reporting instructions, please see:\n");
            printf("[https://github.com/magayaga/v2]\n");
            return 0;
        }
        
        else if (strcmp(argv[i], "--author") == 0) {
            printf("Copyright (c) 2024 Cyril John Magayaga\n");
            return 0;
        }
        
        else if (strcmp(argv[i], "--transpiler::json") == 0) {
            transpileJSON = 1;
        }
        
        else if (strcmp(argv[i], "--transpiler::yaml") == 0) {
            transpileYAML = 1;
        }
        
        else if (strcmp(argv[i], "--load") == 0) {
            if (i + 1 < argc) {
                loadAndInterpret = 1;
                loadFilename = argv[++i];
            }
            
            else {
                fprintf(stderr, "Error: --load option requires a filename\n");
                return 1;
            }
        }
        
        else {
            ConfigItem *config = parseV2Config(argv[i]);
            if (!config) {
                fprintf(stderr, "Failed to parse %s\n", argv[i]);
                continue;
            }

            char jsonFilename[256];
            char yamlFilename[256];

            // Serialize to JSON
            if (transpileJSON) {
                changeFileExtension(argv[i], jsonFilename, ".json");
                FILE *jsonFile = fopen(jsonFilename, "w");
                if (jsonFile) {
                    serializeJSON(config, jsonFile);
                    fclose(jsonFile);
                    printf("Transpiled to JSON: %s\n", jsonFilename);
                }
                
                else {
                    fprintf(stderr, "Failed to open file %s for writing\n", jsonFilename);
                }
            }

            // Serialize to YAML
            if (transpileYAML) {
                changeFileExtension(argv[i], yamlFilename, ".yaml");
                FILE *yamlFile = fopen(yamlFilename, "w");
                if (yamlFile) {
                    serializeYAML(config, yamlFile, 0);
                    fclose(yamlFile);
                    printf("Transpiled to YAML: %s\n", yamlFilename);
                }
                
                else {
                    fprintf(stderr, "Failed to open file %s for writing\n", yamlFilename);
                }
            }

            freeConfigItem(config);
        }
    }

    if (loadAndInterpret && loadFilename) {
        ConfigItem *config = parseV2Config(loadFilename);
        if (!config) {
            fprintf(stderr, "Failed to load %s\n", loadFilename);
            return 1;
        }
        printf("Interpreting %s:\n", loadFilename);
        interpretConfig(config, 0);
        freeConfigItem(config);
    }

    return 0;
}
