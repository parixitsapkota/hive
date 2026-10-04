#ifndef HIVE_CONFIG_H
#define HIVE_CONFIG_H

#define TRI_EXT ".b"

#define NAME_MAX_LEN 256
#define PATH_MAX_LEN 4096

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PATH_SEP "\\"
#define NASM_FMT "win64"
#define OBJ_FMT ".obj"
#define IS_SEP(c) ((c) == '/' || (c) == '\\')
#define CASE_INSENSITIVE_EXT 1
#define LINKER "cc"
#define LINKER_FLAGS
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define PATH_SEP "/"
#define NASM_FMT "elf64"
#define OBJ_FMT ".o"
#define IS_SEP(c) ((c) == '/')
#define CASE_INSENSITIVE_EXT 0
#define LINKER "ld"
#define LINKER_FLAGS
#endif

#ifndef NASM_FMT
#define NASM_FMT "elf64"
#endif

#ifndef OBJ_FMT
#define OBJ_FMT ".o"
#endif

#ifndef LINKER
#define LINKER "ld"
#endif

#ifndef LINKER_FLAGS
#define LINKER_FLAGS
#endif

#endif
