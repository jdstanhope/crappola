#include "crappola.h"
#include <stdarg.h>

#define MAX_VARS 100

typedef struct {
    char *name;
    int offset;
} Variable;

static Variable variables[MAX_VARS];
static int var_count = 0;
static int stack_offset = 0;
static int label_counter = 0;

static char *output = NULL;
static size_t output_size = 0;
static size_t output_capacity = 0;

static void emit(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (output_size + len + 1 >= output_capacity) {
        output_capacity = (output_capacity == 0) ? 4096 : output_capacity * 2;
        output = realloc(output, output_capacity);
    }

    strcpy(output + output_size, buffer);
    output_size += len;
}

static int find_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].offset;
        }
    }
    return -1;
}

static int add_variable(const char *name) {
    int offset = find_variable(name);
    if (offset != -1) {
        return offset;
    }

    stack_offset += 8;
    variables[var_count].name = strdup(name);
    variables[var_count].offset = stack_offset;
    var_count++;
    return stack_offset;
}

static void clear_variables(void) {
    for (int i = 0; i < var_count; i++) {
        free(variables[i].name);
    }
    var_count = 0;
    stack_offset = 0;
}

static int next_label(void) {
    return label_counter++;
}

static void generate_expression(ASTNode *node);

static void generate_statement(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_RETURN:
            generate_expression(node->data.return_stmt.expr);
            emit("    movq %%rbp, %%rsp\n");
            emit("    popq %%rbp\n");
            emit("    ret\n");
            break;

        case NODE_ASSIGNMENT: {
            int offset = add_variable(node->data.assignment.name);
            generate_expression(node->data.assignment.value);
            emit("    movq %%rax, -%d(%%rbp)\n", offset);
            break;
        }

        case NODE_IF: {
            int end_label = next_label();
            int else_label = next_label();
            
            generate_expression(node->data.if_stmt.condition);
            emit("    cmpq $0, %%rax\n");
            if (node->data.if_stmt.else_branch) {
                emit("    je .L%d\n", else_label);
            } else {
                emit("    je .L%d\n", end_label);
            }
            
            generate_statement(node->data.if_stmt.then_branch);
            
            if (node->data.if_stmt.else_branch) {
                emit("    jmp .L%d\n", end_label);
                emit(".L%d:\n", else_label);
                generate_statement(node->data.if_stmt.else_branch);
            }
            
            emit(".L%d:\n", end_label);
            break;
        }

        case NODE_WHILE: {
            int start_label = next_label();
            int end_label = next_label();
            
            emit(".L%d:\n", start_label);
            generate_expression(node->data.while_stmt.condition);
            emit("    cmpq $0, %%rax\n");
            emit("    je .L%d\n", end_label);
            
            generate_statement(node->data.while_stmt.body);
            emit("    jmp .L%d\n", start_label);
            emit(".L%d:\n", end_label);
            break;
        }

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.count; i++) {
                generate_statement(node->data.block.statements[i]);
            }
            break;

        default:
            break;
    }
}

static void generate_expression(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_NUMBER:
            emit("    movq $%d, %%rax\n", node->data.number.value);
            break;

        case NODE_VARIABLE: {
            int offset = find_variable(node->data.variable.name);
            if (offset == -1) {
                fprintf(stderr, "Undefined variable: %s\n", node->data.variable.name);
                return;
            }
            emit("    movq -%d(%%rbp), %%rax\n", offset);
            break;
        }

        case NODE_BINARY_OP:
            generate_expression(node->data.binary_op.right);
            emit("    pushq %%rax\n");
            generate_expression(node->data.binary_op.left);
            emit("    popq %%rcx\n");

            switch (node->data.binary_op.op) {
                case '+':
                    emit("    addq %%rcx, %%rax\n");
                    break;
                case '-':
                    emit("    subq %%rcx, %%rax\n");
                    break;
                case '*':
                    emit("    imulq %%rcx, %%rax\n");
                    break;
                case '/':
                    emit("    cqto\n");
                    emit("    idivq %%rcx\n");
                    break;
                case '<':
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    setl %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
                case '>':
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    setg %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
                case 'l': // <=
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    setle %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
                case 'g': // >=
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    setge %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
                case 'e': // ==
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    sete %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
                case 'n': // !=
                    emit("    cmpq %%rcx, %%rax\n");
                    emit("    setne %%al\n");
                    emit("    movzbq %%al, %%rax\n");
                    break;
            }
            break;

        default:
            break;
    }
}

char *generate_code(ASTNode *ast) {
    if (!ast || ast->type != NODE_FUNCTION) {
        fprintf(stderr, "Invalid AST for code generation\n");
        return NULL;
    }

    output = NULL;
    output_size = 0;
    output_capacity = 0;
    label_counter = 0;
    clear_variables();

#ifdef __APPLE__
    // Emit assembly header for macOS
    emit("    .section __TEXT,__text,regular,pure_instructions\n");
    emit("    .globl _%s\n", ast->data.function.name);
    emit("    .p2align 4, 0x90\n");
    emit("_%s:\n", ast->data.function.name);
#else
    // Emit assembly header for Linux
    emit("    .text\n");
    emit("    .globl %s\n", ast->data.function.name);
    emit("    .type %s, @function\n", ast->data.function.name);
    emit("%s:\n", ast->data.function.name);
#endif
    
    // Function prologue
    emit("    pushq %%rbp\n");
    emit("    movq %%rsp, %%rbp\n");
    
    // Reserve space for local variables (we'll allocate a fixed amount)
    emit("    subq $128, %%rsp\n");
    
    // Generate function body
    generate_statement(ast->data.function.body);
    
    // Default return if no explicit return
    emit("    movq $0, %%rax\n");
    emit("    movq %%rbp, %%rsp\n");
    emit("    popq %%rbp\n");
    emit("    ret\n");

    clear_variables();
    return output;
}
