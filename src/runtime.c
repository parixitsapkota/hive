#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "diag.h"
#include "helpers.h"
#include "runtime.h"

#define LIB_DIR_ENV "HIVE_LIB_DIR"

static const char *const RUNTIME_NAMES[] = {"brt", "libb"};
static const char *const LIB_DIRS[] = {
    "./lib",
    "/usr/local/lib/hive",
    "/usr/lib/hive",
};

size_t runtime_count(void) { return ARRAY_LEN(RUNTIME_NAMES); }

const char *runtime_name(size_t i) {
    return i < ARRAY_LEN(RUNTIME_NAMES) ? RUNTIME_NAMES[i] : NULL;
}

static bool try_lib_dir(const char *dir, const char *file, char *out, size_t n) {
    if (!dir || !*dir) {
        return false;
    }
    int r = snprintf(out, n, "%s/%s", dir, file);
    return r > 0 && (size_t)r < n && access(out, R_OK) == 0;
}

static bool search_lib_dirs(const char *file, char *out, size_t n) {
    for (size_t i = 0; i < ARRAY_LEN(LIB_DIRS); i++) {
        if (try_lib_dir(LIB_DIRS[i], file, out, n)) {
            return true;
        }
    }
    return false;
}

static void print_search_dirs(void) {
    const char *env = getenv(LIB_DIR_ENV);
    fprintf(stderr, "    $%s (%s)\n", LIB_DIR_ENV, env && *env ? env : "unset");
    for (size_t i = 0; i < ARRAY_LEN(LIB_DIRS); i++) {
        fprintf(stderr, "    %s\n", LIB_DIRS[i]);
    }
}

bool runtime_find(const char *name, char *out, size_t out_size) {
    char file[NAME_MAX_LEN];
    if (!append_extension(name, OBJ_FMT, file, sizeof file)) {
        return false;
    }
    if (try_lib_dir(getenv(LIB_DIR_ENV), file, out, out_size) ||
        search_lib_dirs(file, out, out_size)) {
        return true;
    }
    diag_error("cannot find runtime object '%s', searched:", file);
    print_search_dirs();
    return false;
}
