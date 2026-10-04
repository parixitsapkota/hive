#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SHI_STRIP_PREFIX
#include "dep/shi_file.h"
#include "dep/shi_flags.h"

#include "backend.h"
#include "info.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    bool *help = shi_flag_bool("-help", false, "show this output.");
    shi_flag_set_short(help, "h");

    bool *version = shi_flag_bool("-version", false, "show version information.");
    shi_flag_set_short(version, "v");

    char **name = shi_flag_str("-input", NULL, "input file name.");
    shi_flag_set_short(name, "i");

    char **output = shi_flag_str("-output", "out.asm", "output file name.");
    shi_flag_set_short(output, "o");

    char **target = shi_flag_str("-target", "x86_64_nasm", "output x86_64_nasm to output file.");
    shi_flag_set_short(target, "t");

    if (!shi_flag_parse(argc, argv)) {
        shi_flag_print_error(stderr);
        fprintf(stderr, "Usage: %s [OPTIONS]\n", shi_flag_program_name());
        shi_flag_print_options(stderr);
        return 1;
    }

    if (*help) {
        fprintf(stderr, "Usage: %s [OPTIONS]\n", shi_flag_program_name());
        shi_flag_print_options(stderr);
        return 1;
    }

    if (*version) {
        fprintf(stdout, BOLD FG_BLUE "Hive " RESET VERSION_INFO "\n");
        fprintf(stdout, DIM BOLD "Compiler : " RESET CC_INFO "\n");
        fprintf(stdout, DIM BOLD "Built    : " RESET TIME_INFO "\n");
        return 0;
    }

    if (!*name) {
        fprintf(stderr, "%s : No input file provided\n", shi_flag_program_name());
        fprintf(stderr, "Usage: %s -i <input.b>\n", shi_flag_program_name());
        return 1;
    }

    const char *file_path = *name;

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        fprintf(stderr, "FATAL : Failed to open file: %s\n", file_path);
        return 1;
    }
    size_t buff_len = 0;
    char *buffer = read_file(file, &buff_len);
    fclose(file);

    Lexer *l = init_lexer(file_path, buffer, buff_len);
    lexer(l);
    free(buffer);

    Parser *p = init_parser(l);
    parser(p);

    Ir *ir = init_ir(p, *output);
    gen_ir(ir);
    Targets target_kind = target_string_to_kind(*target);
    gen_output(ir, target_kind, *output);
    free_ir(ir);

    free_lexer(l);
    free_parser(p);
    return 0;
}

#define SHI_ARENA_IMPLEMENTATION
#include "dep/shi_arena.h"
#define SHI_FILE_IMPLEMENTATION
#include "dep/shi_file.h"
#define SHI_FLAGS_IMPLEMENTATION
#include "dep/shi_flags.h"
#define SHI_HS_IMPLEMENTATION
#include "dep/shi_hs.h"
