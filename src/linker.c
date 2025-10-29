#include "crappola.h"
#include <unistd.h>
#include <sys/wait.h>

int link_program(const char *asm_file, const char *output_file) {
    // First, assemble the .s file to .o file
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "/tmp/crappola_%d.o", getpid());

    pid_t as_pid = fork();
    if (as_pid == 0) {
        // Child process: run assembler
#ifdef __APPLE__
        execlp("as", "as", "-arch", "x86_64", "-o", obj_file, asm_file, NULL);
#else
        execlp("as", "as", "-o", obj_file, asm_file, NULL);
#endif
        fprintf(stderr, "Error: Failed to execute assembler\n");
        exit(1);
    } else if (as_pid < 0) {
        fprintf(stderr, "Error: Failed to fork for assembler\n");
        return -1;
    }

    // Wait for assembler
    int as_status;
    waitpid(as_pid, &as_status, 0);
    if (WIFEXITED(as_status) && WEXITSTATUS(as_status) != 0) {
        fprintf(stderr, "Error: Assembler failed\n");
        return -1;
    }

    // Link the object file
    pid_t ld_pid = fork();
    if (ld_pid == 0) {
        // Child process: run linker
#ifdef __APPLE__
        execlp("ld", "ld", 
               "-arch", "x86_64",
               "-macosx_version_min", "10.13",
               "-lSystem",
               "-o", output_file,
               obj_file,
               NULL);
#else
        execlp("ld", "ld", 
               "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2",
               "-o", output_file,
               "/usr/lib/x86_64-linux-gnu/crt1.o",
               "/usr/lib/x86_64-linux-gnu/crti.o",
               obj_file,
               "-lc",
               "/usr/lib/x86_64-linux-gnu/crtn.o",
               NULL);
#endif
        fprintf(stderr, "Error: Failed to execute linker\n");
        exit(1);
    } else if (ld_pid < 0) {
        fprintf(stderr, "Error: Failed to fork for linker\n");
        remove(obj_file);
        return -1;
    }

    // Wait for linker
    int ld_status;
    waitpid(ld_pid, &ld_status, 0);
    remove(obj_file);

    if (WIFEXITED(ld_status) && WEXITSTATUS(ld_status) != 0) {
        fprintf(stderr, "Error: Linker failed\n");
        return -1;
    }

    return 0;
}
