/*
 *
 * V2 - VILLAMER AND VALENCIA
 * This is a configuration as code language with powerful tooling.
 * Copyright (c) 2024 Cyril John Magayaga
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
    if (item->key) free(item->key);
    if (item->value) free(item->value);
    if (item->child) freeConfigItem(item->child);
    if (item->next) freeConfigItem(item->next);
    free(item);
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
            current = stack[--stackIndex];
        }
    }

    fclose(file);
    return root;
}

// Function to escape JSON strings
void escapeJSONString(const char *input, char *output) {
    while (*input) {
        switch (*input) {
            case '"':
                *output++ = '\\';
                *output++ = '"';
                break;
            case '\\':
                *output++ = '\\';
                *output++ = '\\';
                break;
            case '\b':
                *output++ = '\\';
                *output++ = 'b';
                break;
            case '\f':
                *output++ = '\\';
                *output++ = 'f';
                break;
            case '\n':
                *output++ = '\\';
                *output++ = 'n';
                break;
            case '\r':
                *output++ = '\\';
                *output++ = 'r';
                break;
            case '\t':
                *output++ = '\\';
                *output++ = 't';
                break;
            default:
                *output++ = *input;
                break;
        }
        input++;
    }
    *output = '\0';
}

// Function to serialize a ConfigItem to JSON
void serializeJSON(ConfigItem *item, FILE *file) {
    if (!item || !item->child) return;

    ConfigItem *child = item->child;
    fprintf(file, "{");
    int first = 1;  // Flag to check if it is the first element
    while (child) {
        if (!first) {
            fprintf(file, ",");
        }
        first = 0;

        // Escape key and value
        char escapedKey[256], escapedValue[256];
        escapeJSONString(child->key, escapedKey);
        fprintf(file, "\"%s\": ", escapedKey);

        if (child->child) {
            serializeJSON(child, file);
        }
        
        else {
            if (child->value) {
                escapeJSONString(child->value, escapedValue);
                fprintf(file, "\"%s\"", escapedValue);
            } else {
                fprintf(file, "null");
            }
        }
        child = child->next;
    }
    fprintf(file, "}");
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

// Main function to process multiple .v2 files
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1.v2> <file2.v2> ...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        ConfigItem *config = parseV2Config(argv[i]);
        if (!config) {
            fprintf(stderr, "Failed to parse %s\n", argv[i]);
            continue;
        }

        char jsonFilename[256];
        char yamlFilename[256];

        changeFileExtension(argv[i], jsonFilename, ".json");
        changeFileExtension(argv[i], yamlFilename, ".yaml");

        // Serialize to JSON
        FILE *jsonFile = fopen(jsonFilename, "w");
        if (jsonFile) {
            serializeJSON(config, jsonFile);
            fclose(jsonFile);
        }
        
        else {
            fprintf(stderr, "Failed to open file %s for writing\n", jsonFilename);
        }

        // Serialize to YAML
        FILE *yamlFile = fopen(yamlFilename, "w");
        if (yamlFile) {
            serializeYAML(config, yamlFile, 0);
            fclose(yamlFile);
        }
        
        else {
            fprintf(stderr, "Failed to open file %s for writing\n", yamlFilename);
        }

        // Clean up
        freeConfigItem(config);
    }

    return 0;
}
