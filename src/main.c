#include "crappola.h"
#include <unistd.h>

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
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

static int write_file(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not write to file %s\n", filename);
        return -1;
    }

    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.c> [-o output]\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = "a.out";

    // Parse output file option
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            output_file = argv[i + 1];
            break;
        }
    }

    printf("Crappola C Compiler v0.1\n");
    printf("Compiling: %s\n", input_file);

    // Step 1: Read source file
    char *source = read_file(input_file);
    if (!source) {
        return 1;
    }

    // Step 2: Preprocess
    printf("  [1/5] Preprocessing...\n");
    char *preprocessed = preprocess(source);
    free(source);
    if (!preprocessed) {
        return 1;
    }

    // Step 3: Tokenize (Lexer)
    printf("  [2/5] Lexical analysis...\n");
    int token_count = 0;
    Token *tokens = tokenize(preprocessed, &token_count);
    free(preprocessed);
    if (!tokens) {
        return 1;
    }

    // Step 4: Parse (Parser)
    printf("  [3/5] Parsing...\n");
    ASTNode *ast = parse(tokens, token_count);
    free_tokens(tokens, token_count);
    if (!ast) {
        return 1;
    }

    // Step 5: Generate assembly code
    printf("  [4/5] Code generation...\n");
    char *assembly = generate_code(ast);
    free_ast(ast);
    if (!assembly) {
        return 1;
    }

    // Write assembly to temporary file
    char asm_file[256];
    snprintf(asm_file, sizeof(asm_file), "/tmp/crappola_%d.s", getpid());
    if (write_file(asm_file, assembly) != 0) {
        free(assembly);
        return 1;
    }
    free(assembly);

    // Step 6: Link
    printf("  [5/5] Linking...\n");
    if (link_program(asm_file, output_file) != 0) {
        remove(asm_file);
        return 1;
    }

    remove(asm_file);
    printf("Success! Output: %s\n", output_file);

    return 0;
}
