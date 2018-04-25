#include "neotags.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STARTSIZE 2048
#define INCSIZE   256
#define GUESS 100
#define INC   10

#define safe_stat(PATH, ST)                                         \
     do {                                                           \
             if ((stat((PATH), (ST)) != 0))                         \
                     xperror("Failed to stat file '%s", (PATH));    \
     } while (0)

static inline bool file_is_reg(const char *filename);


int
my_fgetline(char **ptr, FILE *fp)
{
        int ch, it, size;
        char *buf, *temp;

        buf = temp = NULL;
        buf  = xmalloc(STARTSIZE);
        size = STARTSIZE;

        it = 0;
        while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
                if (ch == '\r')
                        continue;
                if (it >= (size - 1)) {
                        size += INCSIZE;
                        temp = xrealloc((void*)buf, size);
                        buf = temp;
                }

                buf[it++] = ch;
        }

        if (ch == EOF && it == 0) {
                free(buf);
                *ptr = NULL;
                return 0;
        }

        buf[it++] = '\0';

        temp = xrealloc(buf, it);
        *ptr = temp;

        return it;
}



struct strlst *
get_all_lines(const char *filename)
{
        FILE *fp = safe_fopen(filename, "r");
        struct strlst *vec = xmalloc(sizeof *vec);
        uint32_t max = GUESS;
        {
                char **strvec = xmalloc(sizeof *strvec * max);
                uint32_t *intvec = xmalloc(sizeof *intvec * max);
                vec->s = strvec;
                vec->slen = intvec;
                vec->num = 0;
        }

        do {
                if (vec->num == max) {
                        max += INC;
                        vec->slen = xrealloc(vec->slen,
                                             sizeof *vec->slen * max);
                        vec->s = xrealloc(vec->s,
                                          sizeof *vec->s * max);
                }
                vec->slen[vec->num] = my_fgetline((vec->s + vec->num), fp);

        } while (vec->slen[vec->num++] > 0);

        vec->slen = xrealloc(vec->slen, sizeof *vec->slen * vec->num);
        vec->s = xrealloc(vec->s, sizeof *vec->s * vec->num);
        --vec->num;

        fclose(fp);
        return vec;
}


FILE *
safe_fopen(const char * const restrict filename,
           const char * const restrict mode)
{
        FILE *fp = fopen(filename, mode);
        if (!fp)
                xperror("Failed to open file '%s'", filename);
        if (!file_is_reg(filename))
                xerr(1, "Invalid filetype '%s'\n", filename);

        return fp;
}


static inline bool
file_is_reg(const char *filename)
{
        struct stat st;
        safe_stat(filename, &st);
        return S_ISREG(st.st_mode);
}


void
destroy_strlst(struct strlst *vec)
{
        for (uint32_t i = 0; i < vec->num; ++i)
                free(vec->s[i]);
        free(vec->s);
        free(vec->slen);
        free(vec);
}


void *
xmalloc(const size_t size)
{
        void *tmp = malloc(size);
        if (tmp == NULL)
                xerr(100, "Malloc call failed - attempted %zu bytes.\n", size);
        return tmp;
}


void *
xcalloc(const int num, const size_t size)
{
        void *tmp = calloc(num, size);
        if (tmp == NULL)
                xerr(125, "Calloc call failed - attempted %zu bytes.\n", size);
        return tmp;
}


void *
xrealloc(void *ptr, const size_t size)
{
        void *tmp = realloc(ptr, size);
        if (tmp == NULL)
                xerr(150, "Realloc call failed - attempted %zu bytes.\n", size);
        return tmp;
}


int
xasprintf(char ** restrict ptr, const char * restrict fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        int size;

        if ((size = vasprintf(ptr, fmt, ap)) < 0)
                xerr(2, "Error allocating memory for asprintf.\n");

        va_end(ap);
        return size;
}


int64_t
__xatoi(char *str, bool strict)
{
        char *endptr;
        long int val = strtol(str, &endptr, 10);

        if ((endptr == str) || (strict && *endptr != '\0'))
                xerr(30, "Invalid integer '%s'.\n", str);

        return (int)val;
}


void
dump_list(char **list, FILE *fp)
{
        char *tmp, **lcpy = list;
        while ((tmp = *lcpy++) != NULL)
                fprintf(fp, "%s\n", tmp);
}
