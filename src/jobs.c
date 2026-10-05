#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "backend.h"
#include "compile.h"
#include "config.h"
#include "diag.h"
#include "helpers.h"
#include "jobs.h"
#include "runtime.h"

#define MAX_THREADS 64

typedef struct {
    const char *const *inputs;
    size_t n;
    bool compile_only;
    bool dbg;
    bool combine_o;
    bool keep_temps;
    const char *output;
    Targets kind;
    size_t threads;
    char (*stems)[NAME_MAX_LEN];
    char (*objs)[PATH_MAX_LEN];
    size_t n_objs;
    char tmp_dir[PATH_MAX_LEN];
    bool have_tmp;
} Job;

typedef struct {
    Job *job;
    atomic_size_t next;
    atomic_bool failed;
} Pool;

static void job_free(Job *job) {
    if (job->have_tmp && job->keep_temps) {
        fprintf(stderr, "%s : intermediate files kept in %s\n", diag_prog(), job->tmp_dir);
    } else if (job->have_tmp) {
        remove_temp_dir(job->tmp_dir);
    }
    free(job->stems);
    free(job->objs);
}

static bool job_init(Job *job, const JobSpec *spec) {
    memset(job, 0, sizeof *job);
    if (spec->n_inputs == 0) {
        return diag_error("No input file provided");
    }
    job->inputs = spec->inputs;
    job->n = spec->n_inputs;
    job->compile_only = spec->compile_only;
    job->keep_temps = spec->keep_temps;
    job->output = spec->output;
    job->kind = spec->kind;
    job->threads = spec->threads;
    job->stems = calloc(job->n, sizeof *job->stems);
    job->objs = calloc(job->n + runtime_count(), sizeof *job->objs);
    job->n_objs = job->n;
    job->dbg = spec->dbg;
    job->dbg = spec->combine_o;
    if (job->stems && job->objs) {
        return true;
    }
    job_free(job);
    return diag_error("out of memory");
}

static bool get_stem(const char *path, char *stem, size_t n) {
    return path_filename_copy(path, stem, n) && tri_strip_extension(stem, stem, n);
}

static bool validate_inputs(Job *job) {
    bool ok = true;
    for (size_t i = 0; i < job->n; i++) {
        const char *path = job->inputs[i];
        if (has_tri_extension(path) && get_stem(path, job->stems[i], NAME_MAX_LEN)) {
            continue;
        }
        diag_error("%s : file format not recognized!", path);
        ok = false;
    }
    return ok;
}

static bool has_duplicate_output(const Job *job, size_t i) {
    for (size_t j = i + 1; j < job->n; j++) {
        if (strcmp(job->stems[i], job->stems[j]) != 0) {
            continue;
        }
        diag_error("'%s' and '%s' would both produce %s%s", job->inputs[i], job->inputs[j],
                   job->stems[i], OBJ_FMT);
        return true;
    }
    return false;
}

static bool check_unique_outputs(const Job *job) {
    if (!job->compile_only) {
        return true;
    }
    for (size_t i = 0; i < job->n; i++) {
        if (has_duplicate_output(job, i)) {
            return false;
        }
    }
    return true;
}

static bool is_input_stem(const Job *job, const char *name) {
    for (size_t i = 0; i < job->n; i++) {
        if (strcmp(job->stems[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static bool add_runtime_object(Job *job, const char *name) {
    if (is_input_stem(job, name)) {
        return true;
    }
    if (!runtime_find(name, job->objs[job->n_objs], PATH_MAX_LEN)) {
        return false;
    }
    job->n_objs++;
    return true;
}

static bool find_runtime(Job *job) {
    if (job->compile_only || job->combine_o) {
        return true;
    }
    for (size_t i = 0; i < runtime_count(); i++) {
        if (!add_runtime_object(job, runtime_name(i))) {
            return false;
        }
    }
    return true;
}

static bool make_path(char *dst, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(dst, n, fmt, ap);
    va_end(ap);
    return (r >= 0 && (size_t)r < n) || diag_error("path too long");
}

static bool make_obj_path(Job *job, size_t i) {
    if (job->compile_only) {
        return append_extension(job->stems[i], OBJ_FMT, job->objs[i], PATH_MAX_LEN) != 0 ||
               diag_error("path too long");
    }
    return make_path(job->objs[i], PATH_MAX_LEN, "%s/%zu_%s%s", job->tmp_dir, i, job->stems[i],
                     OBJ_FMT);
}

static bool compile_unit(Job *job, size_t i) {
    const char *src = job->inputs[i];
    char asm_path[PATH_MAX_LEN];

    if (!make_path(asm_path, sizeof asm_path, "%s/%zu_%s.asm", job->tmp_dir, i, job->stems[i]) ||
        !make_obj_path(job, i)) {
        return false;
    }
    if (!compile_to_asm(src, asm_path, job->kind)) {
        return diag_error("%s : compilation failed", src);
    }
    if (!assemble(asm_path, job->objs[i], job->dbg)) {
        return diag_error("%s : assembling failed", src);
    }
    return true;
}

static bool claim_unit(Pool *pool, size_t *i) {
    if (atomic_load(&pool->failed)) {
        return false;
    }
    *i = atomic_fetch_add(&pool->next, 1);
    return *i < pool->job->n;
}

static void *worker(void *arg) {
    Pool *pool = arg;
    size_t i;
    while (claim_unit(pool, &i)) {
        if (!compile_unit(pool->job, i)) {
            atomic_store(&pool->failed, true);
        }
    }
    return NULL;
}

static size_t pick_threads(const Job *job) {
    size_t t = job->threads;
    if (t == 0) {
        long cpus = sysconf(_SC_NPROCESSORS_ONLN);
        t = cpus > 0 ? (size_t)cpus : 1;
    }
    if (t > MAX_THREADS) {
        t = MAX_THREADS;
    }
    return t < job->n ? t : job->n;
}

static size_t spawn_workers(Pool *pool, pthread_t *tids, size_t count) {
    size_t started = 0;
    while (started < count && pthread_create(&tids[started], NULL, worker, pool) == 0) {
        started++;
    }
    return started;
}

static void join_workers(const pthread_t *tids, size_t started) {
    for (size_t i = 0; i < started; i++) {
        pthread_join(tids[i], NULL);
    }
}

static bool compile_parallel(Job *job) {
    size_t nthreads = pick_threads(job);
    Pool pool = {.job = job};
    atomic_init(&pool.next, 0);
    atomic_init(&pool.failed, false);

    pthread_t *tids = calloc(nthreads, sizeof *tids);
    size_t started = tids ? spawn_workers(&pool, tids, nthreads - 1) : 0;
    worker(&pool);
    join_workers(tids, started);
    free(tids);
    return !atomic_load(&pool.failed);
}

static bool compile_all(Job *job) {
    if (!create_temp_dir(job->tmp_dir, sizeof job->tmp_dir)) {
        return diag_error("failed to create temporary directory");
    }
    job->have_tmp = true;
    return compile_parallel(job);
}

static bool link_all(const Job *job) {
    if (job->compile_only) {
        return true;
    }
    const char *exe = job->output ? job->output : job->stems[0];
    if (link_objects(exe, job->objs, job->n_objs, job->dbg, job->combine_o)) {
        return true;
    }
    return diag_error("linking failed");
}

int jobs_run(const JobSpec *spec) {
    Job job;
    if (!job_init(&job, spec)) {
        return 1;
    }
    bool ok = validate_inputs(&job) && check_unique_outputs(&job) && find_runtime(&job) &&
              compile_all(&job) && link_all(&job);
    job_free(&job);
    return ok ? 0 : 1;
}
