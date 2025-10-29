#include "crappola.h"
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

/* Preprocessor that handles directives including #include */

#define MAX_DEFINES 100
#define MAX_LINE 1024
#define MAX_INCLUDE_DEPTH 100

typedef struct {
    char *name;
    char *value;
} Define;

typedef struct IncludeNode {
    char *filename;
    struct IncludeNode *parent;
} IncludeNode;

static Define defines[MAX_DEFINES];
static int define_count = 0;

static void add_define(const char *name, const char *value) {
    if (define_count >= MAX_DEFINES) {
        return;
    }
    defines[define_count].name = strdup(name);
    defines[define_count].value = strdup(value ? value : "");
    define_count++;
}

static const char *get_define(const char *name) {
    for (int i = 0; i < define_count; i++) {
        if (strcmp(defines[i].name, name) == 0) {
            return defines[i].value;
        }
    }
    return NULL;
}

static void clear_defines(void) {
    for (int i = 0; i < define_count; i++) {
        free(defines[i].name);
        free(defines[i].value);
    }
    define_count = 0;
}

/* Check if a file is already in the include chain (cycle detection) */
static bool check_cycle(IncludeNode *chain, const char *filename) {
    while (chain) {
        if (strcmp(chain->filename, filename) == 0) {
            return true;
        }
        chain = chain->parent;
    }
    return false;
}

/* Read file contents */
static char *read_file_contents(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);

    return content;
}

/* Resolve include path relative to base file */
static char *resolve_include_path(const char *base_file, const char *include_file) {
    char *resolved = malloc(PATH_MAX);
    if (!resolved) {
        return NULL;
    }

    // Try relative to base file directory first
    if (base_file) {
        char *base_copy = strdup(base_file);
        char *dir = dirname(base_copy);
        snprintf(resolved, PATH_MAX, "%s/%s", dir, include_file);
        free(base_copy);
        
        if (access(resolved, R_OK) == 0) {
            return resolved;
        }
    }

    // Try as-is
    if (access(include_file, R_OK) == 0) {
        strncpy(resolved, include_file, PATH_MAX - 1);
        resolved[PATH_MAX - 1] = '\0';
        return resolved;
    }

    free(resolved);
    return NULL;
}

/* Internal preprocessing function that handles recursive includes */
static char *preprocess_internal(const char *source, const char *filename, IncludeNode *chain, int depth) {
    if (depth >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: Include depth exceeded maximum (%d)\n", MAX_INCLUDE_DEPTH);
        return NULL;
    }

    size_t buffer_size = strlen(source) * 2 + 1024;
    char *output = malloc(buffer_size);
    if (!output) {
        return NULL;
    }
    output[0] = '\0';

    char *line = malloc(MAX_LINE);
    if (!line) {
        free(output);
        return NULL;
    }

    const char *ptr = source;
    size_t output_len = 0;
    int line_num = 1;

    // Add initial line directive
    char line_directive[512];
    snprintf(line_directive, sizeof(line_directive), "#line 1 \"%s\"\n", filename ? filename : "<input>");
    size_t dir_len = strlen(line_directive);
    if (output_len + dir_len >= buffer_size) {
        buffer_size *= 2;
        char *new_output = realloc(output, buffer_size);
        if (!new_output) {
            free(output);
            free(line);
            return NULL;
        }
        output = new_output;
    }
    strcpy(output + output_len, line_directive);
    output_len += dir_len;

    while (*ptr) {
        // Read a line
        size_t line_len = 0;
        while (*ptr && *ptr != '\n' && line_len < MAX_LINE - 1) {
            line[line_len++] = *ptr++;
        }
        line[line_len] = '\0';
        if (*ptr == '\n') ptr++;

        // Skip leading whitespace
        char *trimmed = line;
        while (*trimmed && isspace(*trimmed)) {
            trimmed++;
        }

        // Handle preprocessor directives
        if (trimmed[0] == '#') {
            char *directive = trimmed + 1;
            while (*directive && isspace(*directive)) {
                directive++;
            }

            // #define
            if (strncmp(directive, "define", 6) == 0) {
                char *def_ptr = directive + 6;
                while (*def_ptr && isspace(*def_ptr)) {
                    def_ptr++;
                }

                char name[256] = {0};
                int name_len = 0;
                while (*def_ptr && (isalnum(*def_ptr) || *def_ptr == '_')) {
                    name[name_len++] = *def_ptr++;
                }
                name[name_len] = '\0';

                while (*def_ptr && isspace(*def_ptr)) {
                    def_ptr++;
                }

                add_define(name, *def_ptr ? def_ptr : "");
                line_num++;
                continue;
            }

            // #include
            if (strncmp(directive, "include", 7) == 0) {
                char *inc_ptr = directive + 7;
                while (*inc_ptr && isspace(*inc_ptr)) {
                    inc_ptr++;
                }

                // Parse include filename (supports "file.h" and <file.h>)
                char include_filename[512] = {0};
                int inc_len = 0;
                char quote_char = '\0';
                
                if (*inc_ptr == '"' || *inc_ptr == '<') {
                    quote_char = (*inc_ptr == '"') ? '"' : '>';
                    inc_ptr++;
                    while (*inc_ptr && *inc_ptr != quote_char && inc_len < 511) {
                        include_filename[inc_len++] = *inc_ptr++;
                    }
                    include_filename[inc_len] = '\0';
                }

                if (include_filename[0] == '\0') {
                    fprintf(stderr, "Error: Invalid #include directive at line %d\n", line_num);
                    free(output);
                    free(line);
                    return NULL;
                }

                // Resolve the include path
                char *resolved_path = resolve_include_path(filename, include_filename);
                if (!resolved_path) {
                    fprintf(stderr, "Error: Could not find included file '%s' at line %d\n", 
                            include_filename, line_num);
                    free(output);
                    free(line);
                    return NULL;
                }

                // Check for cycles
                if (check_cycle(chain, resolved_path)) {
                    fprintf(stderr, "Error: Circular include detected for '%s' at line %d\n", 
                            resolved_path, line_num);
                    free(resolved_path);
                    free(output);
                    free(line);
                    return NULL;
                }

                // Read the included file
                char *included_content = read_file_contents(resolved_path);
                if (!included_content) {
                    fprintf(stderr, "Error: Could not read included file '%s' at line %d\n", 
                            resolved_path, line_num);
                    free(resolved_path);
                    free(output);
                    free(line);
                    return NULL;
                }

                // Create new include node for cycle detection
                IncludeNode new_node;
                new_node.filename = resolved_path;
                new_node.parent = chain;

                // Recursively process the included file
                char *processed_include = preprocess_internal(included_content, resolved_path, &new_node, depth + 1);
                free(included_content);
                
                if (!processed_include) {
                    free(resolved_path);
                    free(output);
                    free(line);
                    return NULL;
                }

                // Append processed include to output
                size_t inc_output_len = strlen(processed_include);
                if (output_len + inc_output_len + 512 >= buffer_size) {
                    buffer_size = output_len + inc_output_len + 1024;
                    char *new_output = realloc(output, buffer_size);
                    if (!new_output) {
                        free(processed_include);
                        free(resolved_path);
                        free(output);
                        free(line);
                        return NULL;
                    }
                    output = new_output;
                }

                strcpy(output + output_len, processed_include);
                output_len += inc_output_len;
                free(processed_include);

                // Add line directive to return to current file
                snprintf(line_directive, sizeof(line_directive), "#line %d \"%s\"\n", 
                         line_num + 1, filename ? filename : "<input>");
                dir_len = strlen(line_directive);
                if (output_len + dir_len >= buffer_size) {
                    buffer_size *= 2;
                    char *new_output = realloc(output, buffer_size);
                    if (!new_output) {
                        free(resolved_path);
                        free(output);
                        free(line);
                        return NULL;
                    }
                    output = new_output;
                }
                strcpy(output + output_len, line_directive);
                output_len += dir_len;

                free(resolved_path);
                line_num++;
                continue;
            }

            // Skip other directives for simplicity
            line_num++;
            continue;
        }

        // Replace defined macros in the line
        char expanded[MAX_LINE * 2];
        expanded[0] = '\0';
        char *exp_ptr = expanded;

        for (size_t i = 0; i < line_len; i++) {
            if (isalpha(line[i]) || line[i] == '_') {
                // Potential identifier
                char ident[256];
                int ident_len = 0;
                size_t j = i;
                while (j < line_len && (isalnum(line[j]) || line[j] == '_')) {
                    ident[ident_len++] = line[j++];
                }
                ident[ident_len] = '\0';

                const char *replacement = get_define(ident);
                if (replacement) {
                    strcpy(exp_ptr, replacement);
                    exp_ptr += strlen(replacement);
                } else {
                    strcpy(exp_ptr, ident);
                    exp_ptr += ident_len;
                }
                i = j - 1;
            } else {
                *exp_ptr++ = line[i];
            }
        }
        *exp_ptr = '\0';

        // Add expanded line to output
        size_t expanded_len = strlen(expanded);
        if (output_len + expanded_len + 2 >= buffer_size) {
            buffer_size *= 2;
            char *new_output = realloc(output, buffer_size);
            if (!new_output) {
                free(output);
                free(line);
                return NULL;
            }
            output = new_output;
        }

        strcpy(output + output_len, expanded);
        output_len += expanded_len;
        output[output_len++] = '\n';
        output[output_len] = '\0';
        line_num++;
    }

    free(line);
    return output;
}

char *preprocess(const char *source, const char *filename) {
    // Create root node for include chain
    IncludeNode root;
    root.filename = filename ? strdup(filename) : strdup("<input>");
    root.parent = NULL;

    char *result = preprocess_internal(source, filename, &root, 0);
    
    free(root.filename);
    clear_defines();
    
    return result;
}
