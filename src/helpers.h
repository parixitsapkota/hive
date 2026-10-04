#ifndef HIVE_HELPERS_H
#define HIVE_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

/* proc_util.c */
bool run_command(const char *cmd, char *const argv[]);
bool create_temp_dir(char *out_path, size_t max_len);
bool remove_temp_dir(const char *path);
const char *errno_string(int err, char *buf, size_t size);

/* path_util.c */
const char *path_filename(const char *path);
bool path_filename_copy(const char *path, char *out, size_t out_size);

bool has_tri_extension(const char *filename);
bool tri_strip_extension(const char *filename, char *out, size_t out_size);
bool append_extension(const char *name, const char *ext, char *out, size_t out_size);

#endif
