#include "neotags.h"
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STARTSIZE 1024
#define GUESS 100
#define INC   10

#define safe_stat(PATH, ST)                            \
    do {                                               \
        if ((stat((PATH), (ST)) != 0))                 \
            err(1, "Failed to stat file '%s", (PATH)); \
    } while (0)


size_t
my_fgetline(char **ptr, void *fp)
{
    int ch;
    char *buf   = xmalloc(STARTSIZE);
    size_t size = STARTSIZE;
    size_t it   = 0;

    while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
        if (ch == '\r') {
            continue;
        }

        if (it >= (size - 1)) {
            buf = xrealloc(buf, (size <<= 1));
        }

        buf[it++] = (char)ch;
    }

    if (ch == EOF && it == 0) {
        free(buf);
        *ptr = NULL;
        return 0;
    }

    buf[it++] = '\0';
    *ptr      = xrealloc(buf, it);

    return it;
}


FILE *
safe_fopen(const char *const __restrict filename,
           const char *const __restrict mode)
{
    FILE *fp = fopen(filename, mode);

    if (!fp) {
        err(1, "Failed to open file '%s'", filename);
    }

    if (!file_is_reg(filename)) {
        errx(1, "Invalid filetype '%s'\n", filename);
    }

    return fp;
}


bool
file_is_reg(const char *filename)
{
    struct stat st = {0, 0, 0, 0, 0, 0, 0, {0, 0}, {0, 0}, {0}, {0}, 0, 0};
    safe_stat(filename, &st);
    return S_ISREG(st.st_mode);
}


void
add_to_list(struct StringLst *list, struct String *str)
{
    if (list->num == (list->max - 1)) {
        list->lst = nrealloc(
            list->lst,
            (list->max = (size_t)((double)list->max * 1.5)),
            sizeof * list->lst
        );
    }

    list->lst[list->num++] = str;
}


void
add_backup(struct Backups *list, void *item)
{
    if (list->num >= (list->max - 1))
        list->lst = nrealloc(list->lst, (list->max <<= 1),
                             sizeof * list->lst);

    list->lst[list->num++] = item;
}


void *
xmalloc(const size_t size)
{
    void *tmp = malloc(size);

    if (tmp == NULL) {
        err(100, "Malloc call failed - attempted %zu bytes", size);
    }

    return tmp;
}


void *
xcalloc(const int num, const size_t size)
{
    void *tmp = calloc(num, size);

    if (tmp == NULL) {
        err(101, "Calloc call failed - attempted %zu bytes", size);
    }

    return tmp;
}


void *
xrealloc(void *ptr, const size_t size)
{
    void *tmp = realloc(ptr, size);

    if (tmp == NULL) {
        err(102, "Realloc call failed - attempted %zu bytes", size);
    }

    return tmp;
}


#ifdef HAVE_REALLOCARRAY
void *
xreallocarray(void *ptr, size_t num, size_t size)
{
    void *tmp = reallocarray(ptr, num, size);

    if (tmp == NULL) {
        err(103, "Realloc call failed - attempted %zu bytes", size);
    }

    return tmp;
}
#endif


int64_t
__xatoi(char *str, const bool strict)
{
    char *endptr;
    const long long int val = strtol(str, &endptr, 10);

    if ((endptr == str) || (strict && *endptr != '\0')) {
        errx(30, "Invalid integer '%s'.\n", str);
    }

    return (int)val;
}


void
__dump_list(char **list, FILE *fp, const char *varname)
{
    char *tmp, **lcpy = list;
    fprintf(fp, "Dumping list: %s\n", varname);

    while ((tmp = *lcpy++) != NULL) {
        fprintf(fp, "        %s\n", tmp);
    }
}


void
__dump_string(char *str, const char *filename, FILE *fp, const char *varname)
{
    FILE *log;

    if (fp != NULL) {
        log = fp;
    } else if (filename != NULL) {
        log = safe_fopen(filename, "wb");
    } else
        errx(1, "%s at line %d: %s", __FILE__, __LINE__,
             strerror(EINVAL));

    eprintf("Dumping string '%s' to file '%s'.\n",
            varname, (filename == NULL ? "anon" : filename));

    fputs(str, log);

    if (filename != NULL) {
        fclose(log);
    }
}


void
__free_all(void *ptr, ...)
{
    va_list ap;
    va_start(ap, ptr);

    do {
        xfree(ptr);
    } while ((ptr = va_arg(ap, void *)) != NULL);

    va_end(ap);
}


#define free_list(LST)                           \
    do {                                     \
        for (int i = 0; i < (LST)->num; ++i) \
            free((LST)->lst[i]);         \
    } while (0)

void
__free_all_strlists(strlist *ptr, ...)
{
    va_list ap;
    va_start(ap, ptr);

    do {
        free_list(ptr);
    } while ((ptr = va_arg(ap, void *)) != NULL);

    va_end(ap);
}


#ifdef DOSISH
char *
basename(char *path)
{
    assert(path != NULL && *path != '\0');
    const size_t len = strlen(path);
    char *ptr = path + len;

    while (*ptr != '/' && *ptr != '\\' && ptr != path) {
        --ptr;
    }

    return (*ptr == '/' || *ptr == '\\') ? ptr + 1 : ptr;
}
#endif


#ifndef HAVE_ERR
void
_warn(const bool print_err, const char *const __restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[BUFSIZ];

    if (print_err)
        snprintf(buf, BUFSIZ, "%s: %s: %s\n", program_name, fmt,
                 strerror(errno));
    else {
        snprintf(buf, BUFSIZ, "%s: %s\n", program_name, fmt);
    }

    vfprintf(stderr, buf, ap);

    va_end(ap);
}
#endif


int
find_num_cpus(void)
{
    #ifdef DOSISH
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
    #elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);

        if (count < 1) {
            count = 1;
        }
    }

    return count;
    #else
    return sysconf(_SC_NPROCESSORS_ONLN);
    #endif
}
