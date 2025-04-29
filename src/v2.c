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
#include <ctype.h>

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
            // Trim trailing whitespace from key
            char *end = key + strlen(key) - 1;
            while (end > key && isspace((unsigned char)*end)) {
                *end-- = '\0';
            }
            
            ConfigItem *item = createConfigItem(key, value);
            if (!item) {
                fclose(file);
                freeConfigItem(root);
                return NULL;
            }
            addChild(current, item);
        }
        
        else if (sscanf(line, " %127[^{] {", key) == 1) {
            // Trim trailing whitespace from key
            char *end = key + strlen(key) - 1;
            while (end > key && isspace((unsigned char)*end)) {
                *end-- = '\0';
            }
            
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
void escapeJSONString(const char *input, char *output, size_t outSize) {
    size_t outIndex = 0;
    
    while (*input && outIndex < outSize - 1) {
        switch (*input) {
            case '"': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = '"'; 
                }
                break;
            case '\\': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = '\\'; 
                }
                break;
            case '\b': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = 'b'; 
                }
                break;
            case '\f': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = 'f'; 
                }
                break;
            case '\n': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = 'n'; 
                }
                break;
            case '\r': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = 'r'; 
                }
                break;
            case '\t': 
                if (outIndex + 2 < outSize) {
                    output[outIndex++] = '\\'; 
                    output[outIndex++] = 't'; 
                }
                break;
            default: 
                output[outIndex++] = *input; 
                break;
        }
        input++;
    }
    output[outIndex] = '\0';
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

// Safely write a key or value to JSON
void writeJSONString(FILE *file, const char *str) {
    char escaped[512]; // Increased buffer size for safety
    escapeJSONString(str, escaped, sizeof(escaped));
    fprintf(file, "\"%s\"", escaped);
}

// Function to determine value type for JSON
int isNumeric(const char *str) {
    if (!str) return 0;
    char *endptr;
    strtod(str, &endptr);
    return *endptr == '\0';
}

int isBoolean(const char *str) {
    if (!str) return 0;
    return (strcmp(str, "true") == 0 || strcmp(str, "false") == 0);
}

int isNull(const char *str) {
    if (!str) return 1;
    return (strcmp(str, "null") == 0);
}

// JSON serialization (children only) with proper formatting
void serializeJSON(ConfigItem *item, FILE *file, int indent, int checkDesign) {
    if (!item || !item->child) {
        fprintf(file, "{}");
        return;
    }

    ConfigItem *child = item->child;
    fprintf(file, "{\n");
    int first = 1;

    // Calculate indentation
    char indentStr[128] = "";
    for (int i = 0; i < indent + 1; i++) {
        strcat(indentStr, "    ");
    }

    while (child) {
        if (!first) fprintf(file, ",\n");
        first = 0;

        // Print indentation
        fprintf(file, "%s", indentStr);
        
        char escapedKey[256];
        escapeJSONString(child->key, escapedKey, sizeof(escapedKey));
        
        int keyCount = countSameKey(item->child, child->key);
        int seenCount = 0;
        ConfigItem *temp = item->child;
        while (temp != child) {
            if (strcmp(temp->key, child->key) == 0) seenCount++;
            temp = temp->next;
        }

        // Write the key
        fprintf(file, "\"%s\": ", escapedKey);

        // Handle arrays (multiple elements with same key)
        if (keyCount > 1 && seenCount == 0) {
            fprintf(file, "[\n%s    ", indentStr);
            int arrayFirst = 1;
            
            // Find all siblings with the same key
            ConfigItem *sibling = child;
            while (sibling && seenCount < keyCount) {
                if (strcmp(sibling->key, child->key) == 0) {
                    if (!arrayFirst) fprintf(file, ",\n%s    ", indentStr);
                    arrayFirst = 0;
                    
                    if (sibling->child) {
                        serializeJSON(sibling, file, indent + 1, checkDesign);
                    }
                    
                    else if (sibling->value) {
                        if (checkDesign) {
                            // Intelligent value type detection for better JSON design
                            if (isNumeric(sibling->value)) {
                                fprintf(file, "%s", sibling->value);
                            }
                            
                            else if (isBoolean(sibling->value)) {
                                fprintf(file, "%s", sibling->value);
                            }
                            
                            else if (isNull(sibling->value)) {
                                fprintf(file, "null");
                            }
                            
                            else {
                                writeJSONString(file, sibling->value);
                            }
                        }
                        
                        else {
                            writeJSONString(file, sibling->value);
                        }
                    }
                    
                    else {
                        fprintf(file, "null");
                    }
                    seenCount++;
                }
                if (seenCount < keyCount) sibling = sibling->next;
            }
            
            fprintf(file, "\n%s]", indentStr);
            
            // Skip all processed items
            while (child && seenCount > 0) {
                if (strcmp(child->key, escapedKey) == 0) seenCount--;
                if (seenCount > 0) child = child->next;
            }
        }
        
        else {
            // Handle single value
            if (child->child) {
                serializeJSON(child, file, indent + 1, checkDesign);
            }
            
            else if (child->value) {
                if (checkDesign) {
                    // Intelligent value type detection for better JSON design
                    if (isNumeric(child->value)) {
                        fprintf(file, "%s", child->value);
                    }
                    
                    else if (isBoolean(child->value)) {
                        fprintf(file, "%s", child->value);
                    }
                    
                    else if (isNull(child->value)) {
                        fprintf(file, "null");
                    }
                    
                    else {
                        writeJSONString(file, child->value);
                    }
                }
                
                else {
                    writeJSONString(file, child->value);
                }
            }
            
            else {
                fprintf(file, "null");
            }
            child = child->next;
        }
    }
    
    // Close the object with proper indentation
    char closeIndentStr[128] = "";
    for (int i = 0; i < indent; i++) {
        strcat(closeIndentStr, "    ");
    }
    fprintf(file, "\n%s}", closeIndentStr);
}

// Function to validate JSON structure
int checkDesignJSON(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s for validation\n", filename);
        return 0;
    }
    
    // Simple validation: check for balanced braces
    int braceCount = 0;
    int bracketCount = 0;
    int inString = 0;
    int escape = 0;
    int error = 0;
    int c;
    
    while ((c = fgetc(file)) != EOF && !error) {
        if (escape) {
            escape = 0;
            continue;
        }
        
        if (c == '\\' && inString) {
            escape = 1;
            continue;
        }
        
        if (c == '"' && !escape) {
            inString = !inString;
            continue;
        }
        
        if (!inString) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
            else if (c == '[') bracketCount++;
            else if (c == ']') bracketCount--;
            
            if (braceCount < 0 || bracketCount < 0) {
                error = 1;
                fprintf(stderr, "Error: Unbalanced braces or brackets in JSON\n");
            }
        }
    }
    
    fclose(file);
    
    if (!error && (braceCount != 0 || bracketCount != 0)) {
        fprintf(stderr, "Error: Unbalanced braces or brackets in JSON\n");
        error = 1;
    }
    
    return !error;
}

// Function to validate YAML structure
int checkDesignYAML(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s for validation\n", filename);
        return 0;
    }
    
    int error = 0;
    int lineNum = 0;
    char line[512];
    int indentLevels[100] = {0}; // Track indentation at each level
    int currentLevel = 0;
    int inMultilineString = 0;
    char stringDelimiter = 0; // ' or " for string delimiters
    
    // Check for common YAML errors
    while (fgets(line, sizeof(line), file) && !error) {
        lineNum++;
        
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#') continue;
        
        // Handle multiline strings
        if (inMultilineString) {
            for (char *p = line; *p; p++) {
                if (*p == '\\' && *(p+1) == stringDelimiter) {
                    p++; // Skip the escaped quote
                    continue;
                }
                if (*p == stringDelimiter) {
                    inMultilineString = 0;
                    break;
                }
            }
            continue;
        }
        
        // Count leading spaces for indentation
        int indent = 0;
        char *p = line;
        while (*p == ' ') {
            indent++;
            p++;
        }
        
        // Check for tab characters (not allowed in YAML)
        if (strchr(line, '\t') != NULL) {
            fprintf(stderr, "Error at line %d: Tab characters are not allowed in YAML\n", lineNum);
            error = 1;
            break;
        }
        
        // Check for strings that might need quoting
        if (strchr(line, ':') != NULL) {
            char *key = line;
            while (*key && *key != ':') key++;
            
            if (*key == ':' && *(key+1) != '\0' && *(key+1) != '\n') {
                // Skip whitespace after colon
                char *value = key + 1;
                while (*value == ' ') value++;
                
                // Check if we're entering a multiline string
                if (*value == '"' || *value == '\'') {
                    stringDelimiter = *value;
                    char *endQuote = strchr(value + 1, stringDelimiter);
                    if (!endQuote || *(endQuote-1) == '\\') {
                        inMultilineString = 1;
                    }
                }
                
                // Check for common illegal characters in unquoted values
                if (*value != '"' && *value != '\'' && 
                    (strchr(value, '{') || strchr(value, '}') || 
                     strchr(value, '[') || strchr(value, ']') ||
                     strchr(value, '&') || strchr(value, '*'))) {
                    fprintf(stderr, "Warning at line %d: Value may need quotes: %s", lineNum, value);
                }
            }
        }
        
        // Check indentation (must be consistent, typically multiples of 2)
        if (indent % 2 != 0) {
            fprintf(stderr, "Warning at line %d: Indent is not a multiple of 2 spaces\n", lineNum);
        }
        
        // Determine current level based on indentation
        int level = indent / 2;
        
        // Moving to a deeper level - store the new indent value
        if (level > currentLevel) {
            if (level != currentLevel + 1) {
                fprintf(stderr, "Error at line %d: Indentation increased by more than one level\n", lineNum);
                error = 1;
                break;
            }
            indentLevels[level] = indent;
        }
        // Check if we are at a consistent indentation for this level
        else if (level > 0 && indent != indentLevels[level]) {
            fprintf(stderr, "Error at line %d: Inconsistent indentation for this level\n", lineNum);
            error = 1;
            break;
        }
        
        currentLevel = level;
    }
    
    fclose(file);
    
    if (inMultilineString) {
        fprintf(stderr, "Error: Unclosed string literal in YAML\n");
        error = 1;
    }
    
    return !error;
}

// Function to serialize a ConfigItem to YAML
void serializeYAML(ConfigItem *item, FILE *file, int indent) {
    if (!item) return;

    ConfigItem *child = item->child;
    while (child) {
        for (int i = 0; i < indent; ++i) fprintf(file, "  ");
        
        // For YAML, we need to properly quote strings with special characters
        if (child->value) {
            // Check if value needs quoting (contains special chars)
            int needsQuotes = 0;
            const char *specialChars = ":#{}[]&*!|>'\",";
            for (const char *c = child->value; *c; c++) {
                if (strchr(specialChars, *c) || *c <= ' ') {
                    needsQuotes = 1;
                    break;
                }
            }
            
            if (needsQuotes) {
                fprintf(file, "%s: \"", child->key);
                // Escape double quotes in the value
                for (const char *c = child->value; *c; c++) {
                    if (*c == '"') fprintf(file, "\\\"");
                    else fprintf(file, "%c", *c);
                }
                fprintf(file, "\"\n");
            }
            
            else {
                fprintf(file, "%s: %s\n", child->key, child->value);
            }
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
    int checkDesign = 0;
    int checkYAML = 0;
    char *loadFilename = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("%s [v1.0.3]\n", argv[0]);
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
            printf("   --checkDesignJSON          Check, fix, and format JSON output.\n");
            printf("   --checkDesignYAML          Check and validate YAML output.\n");
            printf("   --load [filename]          Load and interpret the .v2 file.\n");
            printf("\nFor bug reporting instructions, please see:\n");
            printf("[https://github.com/magayaga/v2]\n");
            return 0;
        }
        
        else if (strcmp(argv[i], "--author") == 0) {
            printf("Copyright (c) 2024-2025 Cyril John Magayaga\n");
            return 0;
        }
        
        else if (strcmp(argv[i], "--transpiler::json") == 0) {
            transpileJSON = 1;
        }
        
        else if (strcmp(argv[i], "--transpiler::yaml") == 0) {
            transpileYAML = 1;
        }
        
        else if (strcmp(argv[i], "--checkDesignJSON") == 0) {
            checkDesign = 1;
        }
        
        else if (strcmp(argv[i], "--checkDesignYAML") == 0) {
            checkYAML = 1;
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
                    serializeJSON(config, jsonFile, 0, checkDesign);
                    fclose(jsonFile);
                    printf("Transpiled to JSON: %s\n", jsonFilename);
                    
                    // Validate JSON if checkDesign is enabled
                    if (checkDesign) {
                        if (checkDesignJSON(jsonFilename)) {
                            printf("JSON validation passed for %s\n", jsonFilename);
                        } else {
                            printf("Warning: JSON validation failed for %s\n", jsonFilename);
                        }
                    }
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
                    
                    // Validate YAML if checkYAML is enabled
                    if (checkYAML) {
                        if (checkDesignYAML(yamlFilename)) {
                            printf("YAML validation passed for %s\n", yamlFilename);
                        } else {
                            printf("Warning: YAML validation failed for %s\n", yamlFilename);
                        }
                    }
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
