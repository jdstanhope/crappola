#include "crappola.h"

static bool is_keyword(const char *str, TokenType *type) {
    if (strcmp(str, "int") == 0) {
        *type = TOKEN_INT;
        return true;
    } else if (strcmp(str, "return") == 0) {
        *type = TOKEN_RETURN;
        return true;
    } else if (strcmp(str, "if") == 0) {
        *type = TOKEN_IF;
        return true;
    } else if (strcmp(str, "else") == 0) {
        *type = TOKEN_ELSE;
        return true;
    } else if (strcmp(str, "while") == 0) {
        *type = TOKEN_WHILE;
        return true;
    }
    return false;
}

Token *tokenize(const char *source, int *token_count) {
    int capacity = 100;
    Token *tokens = malloc(sizeof(Token) * capacity);
    if (!tokens) {
        return NULL;
    }

    int count = 0;
    int line = 1;
    const char *ptr = source;

    while (*ptr) {
        // Skip whitespace
        while (*ptr && isspace(*ptr)) {
            if (*ptr == '\n') {
                line++;
            }
            ptr++;
        }

        if (!*ptr) break;

        // Handle #line directives
        if (*ptr == '#') {
            const char *directive_start = ptr;
            ptr++; // skip '#'
            
            // Skip whitespace after #
            while (*ptr && isspace(*ptr) && *ptr != '\n') {
                ptr++;
            }
            
            // Check if it's a line directive
            if (strncmp(ptr, "line", 4) == 0) {
                ptr += 4;
                
                // Skip whitespace
                while (*ptr && isspace(*ptr) && *ptr != '\n') {
                    ptr++;
                }
                
                // Read line number
                if (isdigit(*ptr)) {
                    int new_line = 0;
                    while (isdigit(*ptr)) {
                        new_line = new_line * 10 + (*ptr - '0');
                        ptr++;
                    }
                    line = new_line;
                    
                    // Skip the rest of the line directive (including filename)
                    while (*ptr && *ptr != '\n') {
                        ptr++;
                    }
                    if (*ptr == '\n') {
                        ptr++;
                        // Don't increment line here since we just set it
                    }
                    continue;
                }
            }
            
            // If it's not a line directive, treat # as an error
            fprintf(stderr, "Unexpected character: # at line %d\n", line);
            free_tokens(tokens, count);
            return NULL;
        }

        // Ensure capacity
        if (count >= capacity) {
            capacity *= 2;
            Token *new_tokens = realloc(tokens, sizeof(Token) * capacity);
            if (!new_tokens) {
                free_tokens(tokens, count);
                return NULL;
            }
            tokens = new_tokens;
        }

        tokens[count].line = line;

        // Numbers
        if (isdigit(*ptr)) {
            const char *start = ptr;
            while (isdigit(*ptr)) {
                ptr++;
            }
            int len = ptr - start;
            tokens[count].value = malloc(len + 1);
            strncpy(tokens[count].value, start, len);
            tokens[count].value[len] = '\0';
            tokens[count].type = TOKEN_NUMBER;
            count++;
            continue;
        }

        // Identifiers and keywords
        if (isalpha(*ptr) || *ptr == '_') {
            const char *start = ptr;
            while (isalnum(*ptr) || *ptr == '_') {
                ptr++;
            }
            int len = ptr - start;
            char *word = malloc(len + 1);
            strncpy(word, start, len);
            word[len] = '\0';

            TokenType keyword_type;
            if (is_keyword(word, &keyword_type)) {
                tokens[count].type = keyword_type;
                tokens[count].value = word;
            } else {
                tokens[count].type = TOKEN_IDENTIFIER;
                tokens[count].value = word;
            }
            count++;
            continue;
        }

        // Two-character operators
        if (ptr[0] == '=' && ptr[1] == '=') {
            tokens[count].type = TOKEN_EQ;
            tokens[count].value = strdup("==");
            ptr += 2;
            count++;
            continue;
        }
        if (ptr[0] == '!' && ptr[1] == '=') {
            tokens[count].type = TOKEN_NE;
            tokens[count].value = strdup("!=");
            ptr += 2;
            count++;
            continue;
        }
        if (ptr[0] == '<' && ptr[1] == '=') {
            tokens[count].type = TOKEN_LE;
            tokens[count].value = strdup("<=");
            ptr += 2;
            count++;
            continue;
        }
        if (ptr[0] == '>' && ptr[1] == '=') {
            tokens[count].type = TOKEN_GE;
            tokens[count].value = strdup(">=");
            ptr += 2;
            count++;
            continue;
        }

        // Single-character tokens
        switch (*ptr) {
            case '(':
                tokens[count].type = TOKEN_LPAREN;
                tokens[count].value = strdup("(");
                break;
            case ')':
                tokens[count].type = TOKEN_RPAREN;
                tokens[count].value = strdup(")");
                break;
            case '{':
                tokens[count].type = TOKEN_LBRACE;
                tokens[count].value = strdup("{");
                break;
            case '}':
                tokens[count].type = TOKEN_RBRACE;
                tokens[count].value = strdup("}");
                break;
            case ';':
                tokens[count].type = TOKEN_SEMICOLON;
                tokens[count].value = strdup(";");
                break;
            case '+':
                tokens[count].type = TOKEN_PLUS;
                tokens[count].value = strdup("+");
                break;
            case '-':
                tokens[count].type = TOKEN_MINUS;
                tokens[count].value = strdup("-");
                break;
            case '*':
                tokens[count].type = TOKEN_STAR;
                tokens[count].value = strdup("*");
                break;
            case '/':
                tokens[count].type = TOKEN_SLASH;
                tokens[count].value = strdup("/");
                break;
            case '=':
                tokens[count].type = TOKEN_ASSIGN;
                tokens[count].value = strdup("=");
                break;
            case '<':
                tokens[count].type = TOKEN_LT;
                tokens[count].value = strdup("<");
                break;
            case '>':
                tokens[count].type = TOKEN_GT;
                tokens[count].value = strdup(">");
                break;
            case ',':
                tokens[count].type = TOKEN_COMMA;
                tokens[count].value = strdup(",");
                break;
            default:
                fprintf(stderr, "Unexpected character: %c at line %d\n", *ptr, line);
                free_tokens(tokens, count);
                return NULL;
        }
        ptr++;
        count++;
    }

    // Add EOF token
    if (count >= capacity) {
        capacity++;
        Token *new_tokens = realloc(tokens, sizeof(Token) * capacity);
        if (!new_tokens) {
            free_tokens(tokens, count);
            return NULL;
        }
        tokens = new_tokens;
    }

    tokens[count].type = TOKEN_EOF;
    tokens[count].value = NULL;
    tokens[count].line = line;
    count++;

    *token_count = count;
    return tokens;
}

void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}
