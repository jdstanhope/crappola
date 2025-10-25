#include "crappola.h"

static Token *tokens;
static int current;
static int token_count;

static Token *peek(void) {
    if (current < token_count) {
        return &tokens[current];
    }
    return &tokens[token_count - 1];
}

static Token *advance(void) {
    if (current < token_count - 1) {
        return &tokens[current++];
    }
    return &tokens[token_count - 1];
}

static bool match(TokenType type) {
    if (peek()->type == type) {
        advance();
        return true;
    }
    return false;
}

static ASTNode *create_node(NodeType type) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

static ASTNode *parse_expression(void);
static ASTNode *parse_statement(void);

static ASTNode *parse_primary(void) {
    Token *token = peek();

    if (token->type == TOKEN_NUMBER) {
        advance();
        ASTNode *node = create_node(NODE_NUMBER);
        node->data.number.value = atoi(token->value);
        return node;
    }

    if (token->type == TOKEN_IDENTIFIER) {
        advance();
        ASTNode *node = create_node(NODE_VARIABLE);
        node->data.variable.name = strdup(token->value);
        return node;
    }

    if (match(TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression();
        if (!match(TOKEN_RPAREN)) {
            fprintf(stderr, "Expected ')'\n");
            free_ast(expr);
            return NULL;
        }
        return expr;
    }

    fprintf(stderr, "Unexpected token in expression at line %d\n", token->line);
    return NULL;
}

static ASTNode *parse_multiplicative(void) {
    ASTNode *left = parse_primary();
    if (!left) return NULL;

    while (peek()->type == TOKEN_STAR || peek()->type == TOKEN_SLASH) {
        Token *op = advance();
        ASTNode *right = parse_primary();
        if (!right) {
            free_ast(left);
            return NULL;
        }

        ASTNode *node = create_node(NODE_BINARY_OP);
        node->data.binary_op.op = op->value[0];
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_additive(void) {
    ASTNode *left = parse_multiplicative();
    if (!left) return NULL;

    while (peek()->type == TOKEN_PLUS || peek()->type == TOKEN_MINUS) {
        Token *op = advance();
        ASTNode *right = parse_multiplicative();
        if (!right) {
            free_ast(left);
            return NULL;
        }

        ASTNode *node = create_node(NODE_BINARY_OP);
        node->data.binary_op.op = op->value[0];
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_comparison(void) {
    ASTNode *left = parse_additive();
    if (!left) return NULL;

    TokenType type = peek()->type;
    if (type == TOKEN_LT || type == TOKEN_GT || type == TOKEN_LE || 
        type == TOKEN_GE || type == TOKEN_EQ || type == TOKEN_NE) {
        Token *op = advance();
        ASTNode *right = parse_additive();
        if (!right) {
            free_ast(left);
            return NULL;
        }

        ASTNode *node = create_node(NODE_BINARY_OP);
        if (type == TOKEN_EQ) node->data.binary_op.op = 'e';
        else if (type == TOKEN_NE) node->data.binary_op.op = 'n';
        else if (type == TOKEN_LT) node->data.binary_op.op = '<';
        else if (type == TOKEN_GT) node->data.binary_op.op = '>';
        else if (type == TOKEN_LE) node->data.binary_op.op = 'l';
        else if (type == TOKEN_GE) node->data.binary_op.op = 'g';
        
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }

    return left;
}

static ASTNode *parse_expression(void) {
    return parse_comparison();
}

static ASTNode *parse_statement(void) {
    // Return statement
    if (match(TOKEN_RETURN)) {
        ASTNode *node = create_node(NODE_RETURN);
        node->data.return_stmt.expr = parse_expression();
        if (!node->data.return_stmt.expr) {
            free(node);
            return NULL;
        }
        if (!match(TOKEN_SEMICOLON)) {
            fprintf(stderr, "Expected ';' after return\n");
            free_ast(node);
            return NULL;
        }
        return node;
    }

    // If statement
    if (match(TOKEN_IF)) {
        if (!match(TOKEN_LPAREN)) {
            fprintf(stderr, "Expected '(' after if\n");
            return NULL;
        }
        ASTNode *node = create_node(NODE_IF);
        node->data.if_stmt.condition = parse_expression();
        if (!node->data.if_stmt.condition) {
            free(node);
            return NULL;
        }
        if (!match(TOKEN_RPAREN)) {
            fprintf(stderr, "Expected ')' after if condition\n");
            free_ast(node);
            return NULL;
        }
        node->data.if_stmt.then_branch = parse_statement();
        if (!node->data.if_stmt.then_branch) {
            free_ast(node);
            return NULL;
        }
        if (match(TOKEN_ELSE)) {
            node->data.if_stmt.else_branch = parse_statement();
        }
        return node;
    }

    // While statement
    if (match(TOKEN_WHILE)) {
        if (!match(TOKEN_LPAREN)) {
            fprintf(stderr, "Expected '(' after while\n");
            return NULL;
        }
        ASTNode *node = create_node(NODE_WHILE);
        node->data.while_stmt.condition = parse_expression();
        if (!node->data.while_stmt.condition) {
            free(node);
            return NULL;
        }
        if (!match(TOKEN_RPAREN)) {
            fprintf(stderr, "Expected ')' after while condition\n");
            free_ast(node);
            return NULL;
        }
        node->data.while_stmt.body = parse_statement();
        if (!node->data.while_stmt.body) {
            free_ast(node);
            return NULL;
        }
        return node;
    }

    // Block statement
    if (match(TOKEN_LBRACE)) {
        ASTNode *node = create_node(NODE_BLOCK);
        int capacity = 10;
        node->data.block.statements = malloc(sizeof(ASTNode*) * capacity);
        node->data.block.count = 0;

        while (!match(TOKEN_RBRACE)) {
            if (peek()->type == TOKEN_EOF) {
                fprintf(stderr, "Expected '}'\n");
                free_ast(node);
                return NULL;
            }

            ASTNode *stmt = parse_statement();
            if (!stmt) {
                free_ast(node);
                return NULL;
            }

            if (node->data.block.count >= capacity) {
                capacity *= 2;
                node->data.block.statements = realloc(node->data.block.statements, 
                                                       sizeof(ASTNode*) * capacity);
            }
            node->data.block.statements[node->data.block.count++] = stmt;
        }
        return node;
    }

    // Variable declaration
    if (match(TOKEN_INT)) {
        Token *name = peek();
        if (name->type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected identifier after 'int'\n");
            return NULL;
        }
        advance();

        if (match(TOKEN_SEMICOLON)) {
            // Just declaration, no initialization
            return create_node(NODE_BLOCK); // Empty statement
        }

        if (!match(TOKEN_ASSIGN)) {
            fprintf(stderr, "Expected '=' or ';'\n");
            return NULL;
        }

        ASTNode *node = create_node(NODE_ASSIGNMENT);
        node->data.assignment.name = strdup(name->value);
        node->data.assignment.value = parse_expression();
        if (!node->data.assignment.value) {
            free_ast(node);
            return NULL;
        }

        if (!match(TOKEN_SEMICOLON)) {
            fprintf(stderr, "Expected ';'\n");
            free_ast(node);
            return NULL;
        }
        return node;
    }

    // Assignment or expression statement
    if (peek()->type == TOKEN_IDENTIFIER) {
        Token *name = advance();
        if (match(TOKEN_ASSIGN)) {
            ASTNode *node = create_node(NODE_ASSIGNMENT);
            node->data.assignment.name = strdup(name->value);
            node->data.assignment.value = parse_expression();
            if (!node->data.assignment.value) {
                free_ast(node);
                return NULL;
            }
            if (!match(TOKEN_SEMICOLON)) {
                fprintf(stderr, "Expected ';'\n");
                free_ast(node);
                return NULL;
            }
            return node;
        } else {
            fprintf(stderr, "Expected '=' after identifier\n");
            return NULL;
        }
    }

    fprintf(stderr, "Unexpected token in statement at line %d\n", peek()->line);
    return NULL;
}

ASTNode *parse(Token *token_list, int count) {
    tokens = token_list;
    token_count = count;
    current = 0;

    // Parse function: int main() { ... }
    if (!match(TOKEN_INT)) {
        fprintf(stderr, "Expected 'int' for function return type\n");
        return NULL;
    }

    Token *name = peek();
    if (name->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name\n");
        return NULL;
    }
    advance();

    if (!match(TOKEN_LPAREN)) {
        fprintf(stderr, "Expected '(' after function name\n");
        return NULL;
    }

    if (!match(TOKEN_RPAREN)) {
        fprintf(stderr, "Expected ')' - parameters not supported yet\n");
        return NULL;
    }

    if (!match(TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' to start function body\n");
        return NULL;
    }

    // Parse function body
    ASTNode *body = create_node(NODE_BLOCK);
    int capacity = 10;
    body->data.block.statements = malloc(sizeof(ASTNode*) * capacity);
    body->data.block.count = 0;

    while (!match(TOKEN_RBRACE)) {
        if (peek()->type == TOKEN_EOF) {
            fprintf(stderr, "Expected '}' at end of function\n");
            free_ast(body);
            return NULL;
        }

        ASTNode *stmt = parse_statement();
        if (!stmt) {
            free_ast(body);
            return NULL;
        }

        if (body->data.block.count >= capacity) {
            capacity *= 2;
            body->data.block.statements = realloc(body->data.block.statements, 
                                                   sizeof(ASTNode*) * capacity);
        }
        body->data.block.statements[body->data.block.count++] = stmt;
    }

    ASTNode *function = create_node(NODE_FUNCTION);
    function->data.function.name = strdup(name->value);
    function->data.function.body = body;

    return function;
}

void free_ast(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_FUNCTION:
            free(node->data.function.name);
            free_ast(node->data.function.body);
            break;
        case NODE_RETURN:
            free_ast(node->data.return_stmt.expr);
            break;
        case NODE_BINARY_OP:
            free_ast(node->data.binary_op.left);
            free_ast(node->data.binary_op.right);
            break;
        case NODE_VARIABLE:
            free(node->data.variable.name);
            break;
        case NODE_ASSIGNMENT:
            free(node->data.assignment.name);
            free_ast(node->data.assignment.value);
            break;
        case NODE_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch);
            break;
        case NODE_WHILE:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.count; i++) {
                free_ast(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
        default:
            break;
    }

    free(node);
}
