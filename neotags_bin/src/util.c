#include "neotags.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <inttypes.h>
#include <zlib.h>

#define STARTSIZE 2048
#define INCSIZE   256
#define GUESS 100
#define INC   10

#define safe_stat(PATH, ST)                                     \
     do {                                                       \
             if ((stat((PATH), (ST)) != 0))                     \
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
                if (ch == '\r')
                        continue;
                if (it >= (size - 1)) {
                        size += INCSIZE;
                        buf = xrealloc((void*)buf, size);
                }

                buf[it++] = ch;
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


#if 0
struct linked_list *
get_all_lines(const char *filename)
{
        FILE *fp = safe_fopen(filename, "rb");
        struct linked_list *ll = new_list(ST_STRING_FREE);
        struct lldata *str;

        for (;;) {
                str = xmalloc(sizeof * str);
                str->len = my_fgetline(&str->s, fp);

                /* Only EOF will return a size of 0. Even an empty string has
                 * to be size 1 to fit the null char. */
                if (str->len == 0)
                        break;

                ll_add(ll, str);
        }

        fclose(fp);
        return ll;
}


struct linked_list *
llstrsep(struct lldata *buffer)
{
        char *tok, *buf;
        struct linked_list *ll = new_list(ST_STRING_NOFREE);

        /* Set this global pointer so it can be free'd later... */
        evil_global_backup_pointer = buf = buffer->s;

        while ((tok = strsep(&buf, "\n")) != NULL) {
                if (*tok == '\0')
                        continue;
                struct lldata *str = xmalloc(sizeof * str);
                str->len = buf - tok;
                str->s   = tok;

                ll_add(ll, str);
        }

        return ll;
}
#endif


FILE *
safe_fopen(const char * const __restrict filename,
           const char * const __restrict mode)
{
        FILE *fp = fopen(filename, mode);
        if (!fp)
                err(1, "Failed to open file '%s'", filename);
        if (!file_is_reg(filename))
                errx(1, "Invalid filetype '%s'\n", filename);
        return fp;
}


bool
file_is_reg(const char *filename)
{
        struct stat st;
        safe_stat(filename, &st);
        return S_ISREG(st.st_mode);
}


void *
xmalloc(const size_t size)
{
        void *tmp = malloc(size);
        if (tmp == NULL)
                err(100, "Malloc call failed - attempted %zu bytes\n", size);
        return tmp;
}


void *
xcalloc(const int num, const size_t size)
{
        void *tmp = calloc(num, size);
        if (tmp == NULL)
                err(125, "Calloc call failed - attempted %zu bytes\n", size);
        return tmp;
}


void *
xrealloc(void *ptr, const size_t size)
{
        void *tmp = realloc(ptr, size);
        if (tmp == NULL)
                err(150, "Realloc call failed - attempted %zu bytes\n", size);
        return tmp;
}


int64_t
__xatoi(char *str, bool strict)
{
        char *endptr;
        long int val = strtol(str, &endptr, 10);

        if ((endptr == str) || (strict && *endptr != '\0'))
                errx(30, "Invalid integer '%s'.\n", str);

        return (int)val;
}


void
__dump_list(char **list, FILE *fp, const char *varname)
{
        char *tmp, **lcpy = list;
        fprintf(fp, "Dumping list: %s\n", varname);
        while ((tmp = *lcpy++) != NULL)
                fprintf(fp, "        %s\n", tmp);
}


void
__dump_string(char *str, const char *filename, FILE *fp, const char *varname)
{
        FILE *log;

        if (fp != NULL)
                log = fp;
        else if (filename != NULL)
                log = safe_fopen(filename, "wb");
        else
                errx(1, "%s at line %d: %s", __FILE__, __LINE__,
                     strerror(EINVAL));

        eprintf("Dumping string '%s' to file '%s'.\n",
                varname, (filename == NULL ? "anon" : filename));

        fputs(str, log);

        if (filename != NULL)
                fclose(log);
}


void
__free_all(void *ptr, ...)
{
        va_list ap;
        va_start(ap, ptr);

        do free(ptr);
        while ((ptr = va_arg(ap, void *)) != NULL);

        va_end(ap);
}


#ifdef DOSISH
char *
basename(char *path)
{
        assert(path != NULL && *path != '\0');
        size_t len = strlen(path);
        char *ptr = path + len;
        while (*ptr != '/' && *ptr != '\\' && ptr != path)
                --ptr;
        
        return (*ptr == '/' || *ptr == '\\') ? ptr + 1 : ptr;
}
#endif


#ifndef HAVE_ERR
void
_warn(bool print_err, const char *const __restrict fmt, ...)
{
        va_list ap;
        char *buf;
        va_start(ap, fmt);

#ifdef HAVE_ASPRINTF
        if (print_err)
                asprintf(&buf, "%s: %s: %s\n", program_name, fmt,
                         strerror(errno));
        else
                asprintf(&buf, "%s: %s\n", program_name, fmt);
#else
        size_t size;
        if (print_err) {
                char tmp[BUFSIZ];
                /* strerror() is guarenteed to be less than 8192, strcpy is fine. */
                strcpy(tmp, strerror(errno));
                size = strlen(fmt) + strlen(program_name) + strlen(tmp) + 6;
                buf = xmalloc(size);
                snprintf(buf, size, "%s: %s: %s\n", program_name, fmt, tmp);
        } else {
                size = strlen(fmt) + strlen(program_name) + 4;
                buf = xmalloc(size);
                snprintf(buf, size, "%s: %s\n", program_name, fmt);
        }
#endif

        if (buf == NULL) {
                /* Allocation failed: print the original format and a \n. */
                vfprintf(stderr, fmt, ap);
                fputc('\n', stderr);
        } else {
                vfprintf(stderr, buf, ap);
                free(buf);
        }

        va_end(ap);
}
#endif


int
find_num_cpus(void)
{
#ifdef WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
#elif MACOS
        int nm[2];
        size_t len = 4;
        uint32_t count;

        nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);

        if (count < 1) {
                nm[1] = HW_NCPU;
                sysctl(nm, 2, &count, &len, NULL, 0);
                if (count < 1) { count = 1; }
        }
        return count;
#else
        return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
