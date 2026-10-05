#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

#include "dep/shi_file.h"

#include "backend.h"
#include "diag.h"
#include "info.h"
#include "jobs.h"

#define MAX_THREADS_FLAG 1024

void usage(void) {
    const char *prog = shi_flag_program_name();
    fprintf(stderr, "Usage: %s [OPTIONS] -i <input.tri>...\n", prog);
    fprintf(stderr,
            "  %s -i main.b foo.b bar.b -o prog   compile and link into executable 'prog'\n",
            prog);
    fprintf(stderr,
            "  %s -i main.b foo.b bar.b -c        compile each input to its own object file\n",
            prog);
    fprintf(stderr, "  %s -i main.b foo.b bar.b -j 4      compile with 4 threads\n\n", prog);
    shi_flag_print_options(stderr);
}

void print_version(void) {
    fprintf(stdout, BOLD FG_BLUE "Hive " RESET VERSION_INFO "\n");
    fprintf(stdout, DIM BOLD "Compiler : " RESET CC_INFO "\n");
    fprintf(stdout, DIM BOLD "Built    : " RESET TIME_INFO "\n");
}

void define_flags(Options *o) {
    o->help = shi_flag_bool("-help", false, "show this output.");
    shi_flag_set_short(o->help, "h");

    o->version = shi_flag_bool("-version", false, "show version information.");
    shi_flag_set_short(o->version, "v");

    o->compile = shi_flag_bool("-compile-object", false,
                               "compile each input to an object file, do not link.");
    shi_flag_set_short(o->compile, "c");

    o->keep_temps = shi_flag_bool(
        "-keep-temps", false, "keep the intermediate .asm/.o files and print where they are.");
    shi_flag_set_short(o->keep_temps, "k");

    o->dbg = shi_flag_bool("-gen-debug-symbols", false, "generates debug info.");
    shi_flag_set_short(o->dbg, "g");

    o->names = shi_flag_list("-input", "input file(s) name(s).");
    shi_flag_set_short((void *)o->names, "i");

    o->output = shi_flag_str("-output", NULL, "output file name.");
    shi_flag_set_short((void *)o->output, "o");

    o->target = shi_flag_str("-target", "x86_64_nasm", "target backend.");
    shi_flag_set_short((void *)o->target, "t");

    o->threads = shi_flag_size("-jobs", 1, "compile threads.");
    shi_flag_set_short((void *)o->threads, "j");
}

static bool check_options(const Options *o) {
    if (o->names->count == 0) {
        diag_error("No input file provided");
        return diag_error("try -h/--help flag to get help.");
    }
    return true;
}

bool options_to_spec(const Options *o, JobSpec *spec) {
    if (!check_options(o)) {
        return false;
    }
    spec->inputs = (const char *const *)o->names->items;
    spec->n_inputs = o->names->count;
    spec->compile_only = *o->compile;
    spec->keep_temps = *o->keep_temps;
    spec->output = *o->output;
    spec->dbg = *o->dbg;
    spec->combine_o = *o->compile && *o->output;
    *o->compile = !spec->combine_o;
    spec->threads = *o->threads;
    spec->kind = target_string_to_kind(*o->target);
    return true;
}

int main(int argc, char *argv[]) {
    Options opts;
    define_flags(&opts);

    int ac = argc;
    char **av = argv;
    for (;;) {
        if (!shi_flag_parse(ac, av)) {
            shi_flag_print_error(stderr);
            usage();
            return 1;
        }
        int rest = shi_flag_rest_argc();
        if (rest == 0) {
            break;
        }
        char **rest_argv = shi_flag_rest_argv();
        shi_flag_list_append(const char *, opts.names, rest_argv[0]);
        ac = rest - 1;
        av = rest_argv + 1;
    }
    diag_init(shi_flag_program_name());

    if (*opts.help) {
        usage();
        return 0;
    }
    if (*opts.version) {
        print_version();
        return 0;
    }

    JobSpec spec;
    if (!options_to_spec(&opts, &spec)) {
        return 1;
    }
    return jobs_run(&spec);
}

#define SHI_ARENA_IMPLEMENTATION
#include "dep/shi_arena.h"
#define SHI_FILE_IMPLEMENTATION
#include "dep/shi_file.h"
#define SHI_FLAGS_IMPLEMENTATION
#include "dep/shi_flags.h"
#define SHI_HS_IMPLEMENTATION
#include "dep/shi_hs.h"
