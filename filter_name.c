#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

static const char ALLOWED[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
typedef struct { char *text, *lower; } Name;
typedef struct { char *text; size_t count, order; } Key;

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }
static void *resize(void *p, size_t n, size_t size) {
    if (size && n > SIZE_MAX / size) { fputs("Allocation overflow\n", stderr); exit(1); }
    void *q = realloc(p, (n ? n : 1) * size);
    if (!q) die("realloc");
    return q;
}
static char *copy(const char *s) { char *p = strdup(s); if (!p) die("strdup"); return p; }
static int compare_keys(const void *a, const void *b) {
    const Key *x = a, *y = b;
    if (x->count != y->count) return x->count > y->count ? -1 : 1;
    return (x->order > y->order) - (x->order < y->order);
}
static int compare_names(const void *a, const void *b) {
    return strcmp(((const Name *)a)->text, ((const Name *)b)->text);
}
static void free_keys(Key *keys, size_t n) {
    for (size_t i = 0; i < n; ++i) free(keys[i].text);
    free(keys);
}
static void write_names(const char *path, Name *names, size_t *active, size_t n) {
    FILE *f = fopen(path, "w"); if (!f) die(path);
    for (size_t i = 0; i < n; ++i)
        if (fprintf(f, "%s\n", names[active[i]].text) < 0) die("write");
    if (fclose(f)) die("fclose");
}
static size_t filter_by_common_index(int iters, Name *names, size_t n) {
    size_t *active = resize(NULL, n, sizeof(*active));
    for (size_t j = 0; j < n; ++j) active[j] = j;
    size_t nk = sizeof(ALLOWED) - 1;
    Key *keys = resize(NULL, nk, sizeof(*keys));
    for (size_t j = 0; j < nk; ++j) {
        char s[2] = {ALLOWED[j], 0};
        keys[j] = (Key){copy(s), 0, j};
    }
    double fraction = 1.0;
    for (int i = 0; i < iters; ++i, fraction /= 4.0) {
        #pragma omp parallel for schedule(dynamic, 1)
        for (size_t k = 0; k < nk; ++k) {
            keys[k].count = 0;
            keys[k].order = k;
            for (size_t j = 0; j < n; ++j)
                if (strstr(names[active[j]].lower, keys[k].text)) ++keys[k].count;
        }
        qsort(keys, nk, sizeof(*keys), compare_keys);
        size_t nonempty = 0;
        while (nonempty < nk && keys[nonempty].count) ++nonempty;
        printf("top 10: [");
        for (size_t k = 0; k < nonempty && k < 10; ++k)
            printf("%s'%s'", k ? ", " : "", keys[k].text);
        puts("]");
        /* Match int(top_n_percent * len(l) + 0.999) exactly. */
        size_t keep = (size_t)(fraction * (double)nonempty + 0.999);
        unsigned char *matched = resize(NULL, n, sizeof(*matched));
        #pragma omp parallel for schedule(static)
        for (size_t j = 0; j < n; ++j) {
            matched[j] = 0;
            for (size_t k = 0; k < keep; ++k) {
                if (strstr(names[active[j]].lower, keys[k].text)) {
                    matched[j] = 1; break;
                }
            }
        }
        size_t out = 0;
        for (size_t j = 0; j < n; ++j)
            if (matched[j]) active[out++] = active[j];
        free(matched);
        n = out;
        /* Python's set removes duplicates after the first iteration. */
        out = 0;
        for (size_t j = 0; j < n; ++j)
            if (!out || strcmp(names[active[j]].text, names[active[out-1]].text))
                active[out++] = active[j];
        n = out;
        if (i > 2) {
            char path[80];
            snprintf(path, sizeof(path), "usernames_filtered_%d.csv", i);
            write_names(path, names, active, n);
        }
        if (i + 1 < iters) {
            size_t alphabet = sizeof(ALLOWED) - 1;
            if (keep > SIZE_MAX / alphabet) { fputs("Too many keys\n", stderr); exit(1); }
            size_t next_n = keep * alphabet;
            Key *next = resize(NULL, next_n, sizeof(*next));
            size_t t = 0;