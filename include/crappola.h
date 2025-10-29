#ifndef CRAPPOLA_H
#define CRAPPOLA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Token types */
typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_COMMA,
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

/* AST Node types */
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_RETURN,
    NODE_NUMBER,
    NODE_BINARY_OP,
    NODE_VARIABLE,
    NODE_ASSIGNMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_BLOCK,
} NodeType;

/* AST Node */
typedef struct ASTNode {
    NodeType type;
    union {
        struct {
            char *name;
            struct ASTNode *body;
        } function;
        struct {
            struct ASTNode *expr;
        } return_stmt;
        struct {
            int value;
        } number;
        struct {
            char op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        struct {
            char *name;
        } variable;
        struct {
            char *name;
            struct ASTNode *value;
        } assignment;
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        struct {
            struct ASTNode **statements;
            int count;
        } block;
    } data;
    struct ASTNode *next;
} ASTNode;

/* Preprocessor functions */
char *preprocess(const char *source, const char *filename);

/* Lexer functions */
Token *tokenize(const char *source, int *token_count);
void free_tokens(Token *tokens, int count);

/* Parser functions */
ASTNode *parse(Token *tokens, int token_count);
void free_ast(ASTNode *node);

/* Code generator functions */
char *generate_code(ASTNode *ast);

/* Linker functions */
int link_program(const char *asm_file, const char *output_file);

#endif /* CRAPPOLA_H */
