#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define SHI_STRIP_PREFIX
#include "dep/shi_file.h"

#include "backend.h"
#include "compile.h"
#include "config.h"
#include "diag.h"
#include "helpers.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"

#ifndef HIVE_FRONTEND_PARALLEL
static pthread_mutex_t frontend_lock = PTHREAD_MUTEX_INITIALIZER;
#define FRONTEND_LOCK() pthread_mutex_lock(&frontend_lock)
#define FRONTEND_UNLOCK() pthread_mutex_unlock(&frontend_lock)
#else
#define FRONTEND_LOCK() ((void)0)
#define FRONTEND_UNLOCK() ((void)0)
#endif

static bool frontend(const char *src, Targets kind, const char *asm_path) {
    char msg[128];
    FILE *file = fopen(src, "rb");
    if (!file) {
        return diag_error("cannot open '%s': %s", src, errno_string(errno, msg, sizeof msg));
    }
    size_t buff_len = 0;
    char *buffer = read_file(file, &buff_len);
    fclose(file);
    if (!buffer) {
        return diag_error("failed to read '%s'", src);
    }

    Lexer *l = init_lexer(src, buffer, buff_len);
    if (!l) {
        free(buffer);
        return false;
    }
    lexer(l);
    free(buffer);

    Parser *p = init_parser(l);
    if (!p) {
        free_lexer(l);
        return false;
    }
    parser(p);

    Ir *ir = init_ir(p, asm_path);
    if (!ir) {
        free_lexer(l);
        free_parser(p);
        return false;
    }
    gen_ir(ir);
    gen_output(ir, kind, asm_path);
    free_ir(ir);

    free_lexer(l);
    free_parser(p);
    return true;
}

bool compile_to_asm(const char *src, const char *asm_path, Targets kind) {
    FRONTEND_LOCK();
    bool ok = frontend(src, kind, asm_path);
    FRONTEND_UNLOCK();
    return ok;
}

bool assemble(const char *asm_path, const char *obj_path, bool debug) {
    const char *argv[12];
    size_t n = 0;

    argv[n++] = "nasm";
    if (debug) {
        argv[n++] = "-g";
        argv[n++] = "-F";
        argv[n++] = "dwarf";
    }
    argv[n++] = "-f";
    argv[n++] = NASM_FMT;
    argv[n++] = asm_path;
    argv[n++] = "-o";
    argv[n++] = obj_path;
    argv[n] = NULL;

    return run_command("nasm", (char *const *)argv) != 0;
}

bool link_objects(const char *exe, char (*objs)[PATH_MAX_LEN], size_t n, bool debug,
                  bool combine_o) {
    char **argv = malloc((n + 6) * sizeof *argv);
    if (!argv) {
        return diag_error("out of memory");
    }
    size_t k = 0;
    argv[k++] = LINKER;
    if (debug) {
        argv[k++] = "-g";
    }
    if (combine_o) {
        argv[k++] = "-r";
    } else {
        argv[k++] = "-no-pie";
    }
    argv[k++] = "-o";
    argv[k++] = (char *)exe;
    for (size_t i = 0; i < n; i++) {
        argv[k++] = objs[i];
    }
    argv[k] = NULL;
    bool ok = run_command(LINKER, argv) != 0;
    free(argv);
    return ok;
}
