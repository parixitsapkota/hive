#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "helpers.h"

const char *path_filename(const char *path) {
    if (!path) {
        return "";
    }
    const char *name = path;
    for (const char *p = path; *p; p++) {
        if (IS_SEP(*p)) {
            name = p + 1;
        }
    }
#ifdef _WIN32
    if (name == path && path[0] && path[1] == ':') {
        name = path + 2;
    }
#endif
    return name;
}

bool path_filename_copy(const char *path, char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return 0;
    }
    const char *name = path_filename(path);
    size_t len = strlen(name);
    if (len == 0 || len + 1 > out_size) {
        out[0] = '\0';
        return 0;
    }
    memmove(out, name, len + 1);
    return 1;
}

static const char *find_extension(const char *path) {
    const char *base = path;
    for (const char *p = path; *p; p++) {
        if (IS_SEP(*p)) {
            base = p + 1;
        }
    }
    const char *dot = strrchr(base, '.');
    if (dot == NULL || dot == base) {
        return NULL;
    }
    return dot;
}

static bool ext_equals(const char *ext, const char *want) {
    for (; *ext && *want; ext++, want++) {
        char a = *ext;
        char b = *want;
        if (CASE_INSENSITIVE_EXT) {
            if (a >= 'A' && a <= 'Z') {
                a = (char)(a - 'A' + 'a');
            }
            if (b >= 'A' && b <= 'Z') {
                b = (char)(b - 'A' + 'a');
            }
        }
        if (a != b) {
            return false;
        }
    }
    return *ext == *want;
}

bool has_tri_extension(const char *filename) {
    if (!filename) {
        return false;
    }
    const char *ext = find_extension(filename);
    return ext != NULL && ext_equals(ext, TRI_EXT);
}
bool tri_strip_extension(const char *filename, char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return false;
    }
    if (!filename || !has_tri_extension(filename)) {
        out[0] = '\0';
        return false;
    }
    size_t name_len = (size_t)(find_extension(filename) - filename);
    if (name_len + 1 > out_size) {
        out[0] = '\0';
        return false;
    }
    memmove(out, filename, name_len);
    out[name_len] = '\0';
    return true;
}

static int fail(char *out, const char *name) {
    if (out != name) {
        out[0] = '\0';
    }
    return 0;
}

bool append_extension(const char *name, const char *ext, char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return 0;
    }
    if (!name || !ext) {
        return fail(out, name);
    }

    if (*ext == '.') {
        ext++;
    }

    size_t name_len = strlen(name);
    size_t ext_len = strlen(ext);

    if (name_len == 0 || IS_SEP(name[name_len - 1]) || ext_len == 0) {
        return fail(out, name);
    }
    for (size_t i = 0; i < ext_len; i++) {
        if (IS_SEP(ext[i])) {
            return fail(out, name);
        }
    }

    if (name_len > out_size || ext_len > out_size - name_len ||
        out_size - name_len - ext_len < 2) {
        return fail(out, name);
    }

    if (out != name) {
        memcpy(out, name, name_len);
    }
    out[name_len] = '.';
    memmove(out + name_len + 1, ext, ext_len);
    out[name_len + 1 + ext_len] = '\0';
    return 1;
}
