#include "crappola.h"

/* Simple preprocessor that handles basic directives */

#define MAX_DEFINES 100
#define MAX_LINE 1024

typedef struct {
    char *name;
    char *value;
} Define;

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

char *preprocess(const char *source) {
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
                continue;
            }

            // Skip other directives (#include, #ifdef, etc.) for simplicity
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
                clear_defines();
                return NULL;
            }
            output = new_output;
        }

        strcpy(output + output_len, expanded);
        output_len += expanded_len;
        output[output_len++] = '\n';
        output[output_len] = '\0';
    }

    free(line);
    clear_defines();
    return output;
}
