#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PATH_SEP "\\"
#define NASM_FMT "win64"
#define OBJ_FMT ".obj"
#else
#include <dirent.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define PATH_SEP "/"
#define NASM_FMT "elf64"
#define OBJ_FMT ".o"
#endif

#ifdef _WIN32

static char *append_quoted(char *p, const char *arg) {
    if (*arg != '\0' && strpbrk(arg, " \t\n\v\"") == NULL) {
        size_t n = strlen(arg);
        memcpy(p, arg, n);
        return p + n;
    }
    *p++ = '"';
    for (;; arg++) {
        size_t backslashes = 0;
        while (*arg == '\\') {
            arg++;
            backslashes++;
        }
        if (*arg == '\0') {
            memset(p, '\\', backslashes * 2);
            p += backslashes * 2;
            break;
        } else if (*arg == '"') {
            memset(p, '\\', backslashes * 2 + 1);
            p += backslashes * 2 + 1;
            *p++ = '"';
        } else {
            memset(p, '\\', backslashes);
            p += backslashes;
            *p++ = *arg;
        }
    }
    *p++ = '"';
    return p;
}

int run_command(const char *cmd, char *const argv[]) {
    if (!cmd || !*cmd || !argv) {
        fprintf(stderr, "run_command: invalid arguments\n");
        return 0;
    }

    size_t cap = 2 * strlen(cmd) + 3;
    for (size_t i = 1; argv[i]; i++) {
        cap += 2 * strlen(argv[i]) + 3;
    }
    if (cap > 32767) {
        fprintf(stderr, "run_command: command line too long\n");
        return 0;
    }
    char *cmdline = (char *)malloc(cap + 1);
    if (!cmdline) {
        fprintf(stderr, "run_command: out of memory\n");
        return 0;
    }
    char *p = append_quoted(cmdline, cmd);
    for (size_t i = 1; argv[i]; i++) {
        *p++ = ' ';
        p = append_quoted(p, argv[i]);
    }
    *p = '\0';

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    ZeroMemory(&pi, sizeof pi);
    si.cb = sizeof si;

    fflush(stdout);
    fflush(stderr);

    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(cmdline);
    if (!ok) {
        fprintf(stderr, "run_command: failed to start '%s' (error %lu)\n", cmd,
                (unsigned long)GetLastError());
        return 0;
    }

    int success = 0;
    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0) {
        DWORD code = 1;
        if (GetExitCodeProcess(pi.hProcess, &code)) {
            if (code == 0) {
                success = 1;
            } else {
                fprintf(stderr, "run_command: '%s' exited with code %lu\n", cmd,
                        (unsigned long)code);
            }
        }
    } else {
        fprintf(stderr, "run_command: wait failed (error %lu)\n", (unsigned long)GetLastError());
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return success;
}

#else /* POSIX */

bool run_command(const char *cmd, char *const argv[]) {
    char msg[128];
    if (!cmd || !*cmd || !argv || !argv[0]) {
        fprintf(stderr, "run_command: invalid arguments\n");
        return 0;
    }

    fflush(stdout);
    fflush(stderr);

    pid_t pid;
    int err = posix_spawnp(&pid, cmd, NULL, NULL, argv, environ);
    if (err != 0) {
        fprintf(stderr, "run_command: failed to start '%s': %s\n", cmd,
                errno_string(err, msg, sizeof msg));
        return 0;
    }

    int status;
    pid_t w;
    do {
        w = waitpid(pid, &status, 0);
    } while (w < 0 && errno == EINTR);
    if (w < 0) {
        fprintf(stderr, "run_command: waitpid: %s\n", errno_string(errno, msg, sizeof msg));
        return 0;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 0) {
            return 1;
        }
        fprintf(stderr, "run_command: '%s' exited with code %d\n", cmd, code);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "run_command: '%s' killed by signal %d\n", cmd, WTERMSIG(status));
    }
    return 0;
}

#endif

const char *errno_string(int err, char *buf, size_t size) {
    if (!buf || size == 0) {
        return "unknown error";
    }
#if defined(_WIN32)
    return strerror_s(buf, size, err) == 0 ? buf : "unknown error";
#elif defined(__GLIBC__) && defined(_GNU_SOURCE)
    return strerror_r(err, buf, size);
#else
    return strerror_r(err, buf, size) == 0 ? buf : "unknown error";
#endif
}

bool create_temp_dir(char *out_path, size_t max_len) {
    if (!out_path || max_len == 0) {
        return 0;
    }
    out_path[0] = '\0';

#ifdef _WIN32
    char temp_base[MAX_PATH + 1];
    DWORD len = GetTempPathA(MAX_PATH, temp_base);
    if (len == 0 || len > MAX_PATH) {
        fprintf(stderr, "create_temp_dir: GetTempPath failed (error %lu)\n",
                (unsigned long)GetLastError());
        return 0;
    }

    for (unsigned attempt = 0; attempt < 100; attempt++) {
        LARGE_INTEGER ctr;
        QueryPerformanceCounter(&ctr);
        int n = snprintf(out_path, max_len, "%scc_tmp_%lu_%08lx%02x", temp_base,
                         (unsigned long)GetCurrentProcessId(),
                         (unsigned long)(ctr.LowPart ^ GetTickCount()), attempt);
        if (n < 0 || (size_t)n >= max_len) {
            fprintf(stderr, "create_temp_dir: path buffer too small\n");
            out_path[0] = '\0';
            return 0;
        }
        if (CreateDirectoryA(out_path, NULL)) {
            return 1;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            fprintf(stderr, "create_temp_dir: CreateDirectory failed (error %lu)\n",
                    (unsigned long)GetLastError());
            out_path[0] = '\0';
            return 0;
        }
    }
    out_path[0] = '\0';
    return 0;
#else
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir || !*tmpdir) {
        tmpdir = "/tmp";
    }

    size_t dlen = strlen(tmpdir);
    while (dlen > 1 && tmpdir[dlen - 1] == '/') {
        dlen--;
    }

    int n = snprintf(out_path, max_len, "%.*s/cc_tmp_XXXXXX", (int)dlen, tmpdir);
    if (n < 0 || (size_t)n >= max_len) {
        fprintf(stderr, "create_temp_dir: path buffer too small\n");
        out_path[0] = '\0';
        return 0;
    }
    if (mkdtemp(out_path) == NULL) {
        char msg[128];
        fprintf(stderr, "create_temp_dir: mkdtemp('%s') failed: %s\n", out_path,
                errno_string(errno, msg, sizeof msg));
        out_path[0] = '\0';
        return 0;
    }
    return 1;
#endif
}

#ifdef _WIN32

static int remove_tree(const char *path) {
    char pattern[MAX_PATH + 4];
    int n = snprintf(pattern, sizeof pattern, "%s\\*", path);
    if (n < 0 || (size_t)n >= sizeof pattern) {
        return 0;
    }

    int ok = 1;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) {
                continue;
            }
            char child[MAX_PATH + 1];
            n = snprintf(child, sizeof child, "%s\\%s", path, fd.cFileName);
            if (n < 0 || (size_t)n >= sizeof child) {
                ok = 0;
                continue;
            }
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                    ok &= RemoveDirectoryA(child) != 0;
                } else {
                    ok &= remove_tree(child);
                }
            } else {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
                }
                ok &= DeleteFileA(child) != 0;
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return (RemoveDirectoryA(path) != 0) && ok;
}

#else

static int remove_tree(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        return 0;
    }
    int ok = 1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) {
            continue;
        }
        size_t need = strlen(path) + 1 + strlen(e->d_name) + 1;
        char *child = (char *)malloc(need);
        if (!child) {
            ok = 0;
            continue;
        }
        snprintf(child, need, "%s/%s", path, e->d_name);

        struct stat st;
        if (lstat(child, &st) != 0) {
            ok = 0;
        } else if (S_ISDIR(st.st_mode)) {
            ok &= remove_tree(child);
        } else {
            ok &= (unlink(child) == 0);
        }
        free(child);
    }
    closedir(d);
    return (rmdir(path) == 0) && ok;
}

#endif

bool remove_temp_dir(const char *path) {
    if (!path || !*path) {
        return 0;
    }
    return remove_tree(path);
}
