#include "neotags.h"
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

#define safe_stat(PATH, ST)                                         \
     do {                                                           \
             if ((stat((PATH), (ST)) != 0))                         \
                     xperror("Failed to stat file '%s", (PATH));    \
     } while (0)

static bool file_is_reg(const char *filename);
static void safe_gzopen(gzFile *fp, const char *filename, const char *mode);
static int my_gfgetline(char **ptr, gzFile *fp);


static int
my_gfgetline(char **ptr, gzFile *fp)
{
        int ch, it, size;
        char *buf, *temp;

        buf = temp = NULL;
        buf  = xmalloc(STARTSIZE);
        size = STARTSIZE;

        it = 0;
        while ((ch = gzgetc(*fp)) != '\n' && ch != EOF) {
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
        gzFile fp;
        safe_gzopen(&fp, filename, "rb");
        struct strlst *lst = xmalloc(sizeof *lst);
        uint32_t max = GUESS;

        lst->s    = xmalloc(sizeof *lst->s * max);
        lst->slen = xmalloc(sizeof *lst->slen * max);
        lst->num  = 0;

        do {
                if (lst->num == max) {
                        max += INC;
                        lst->slen = xrealloc(lst->slen, sizeof *lst->slen * max);
                        lst->s    = xrealloc(lst->s, sizeof *lst->s * max);
                }
                lst->slen[lst->num] = my_gfgetline((lst->s + lst->num), &fp);

        } while (lst->slen[lst->num++] > 0);

        lst->slen = xrealloc(lst->slen, sizeof *lst->slen * lst->num);
        lst->s = xrealloc(lst->s, sizeof *lst->s * lst->num);
        --lst->num;

        gzclose(fp);
        return lst;
}


static void
safe_gzopen(gzFile *fp,
            const char * const restrict filename,
            const char * const restrict mode)
{
        *fp = gzopen(filename, mode);
        if (!*fp)
                xperror("Failed to open file '%s'", filename);
        if (!file_is_reg(filename))
                xerr(1, "Invalid filetype '%s'\n", filename);
}


static bool
file_is_reg(const char *filename)
{
        struct stat st;
        safe_stat(filename, &st);
        return S_ISREG(st.st_mode);
}


void
destroy_strlst(struct strlst *lst)
{
        for (uint32_t i = 0; i < lst->num; ++i)
                free(lst->s[i]);
        free(lst->s);
        free(lst->slen);
        free(lst);
}


void *
xmalloc(const size_t size)
{
        void *tmp = malloc(size);
        if (tmp == NULL)
                xerr(100, "Malloc call failed - attempted "Psize_t" bytes.\n", size);
        return tmp;
}


void *
xcalloc(const int num, const size_t size)
{
        void *tmp = calloc(num, size);
        if (tmp == NULL)
                xerr(125, "Calloc call failed - attempted "Psize_t" bytes.\n", size);
        return tmp;
}


void *
xrealloc(void *ptr, const size_t size)
{
        void *tmp = realloc(ptr, size);
        if (tmp == NULL)
                xerr(150, "Realloc call failed - attempted "Psize_t" bytes.\n", size);
        return tmp;
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


#if 0
static FILE *
open_gz(const char *filename, const char *mode)
{
        gzFile *zfp;

        /* try gzopen */
        zfp = gzopen(filename, mode);
        if (zfp == NULL)
                return fopen(filename, mode);

        /* open file pointer */
        return funopen(zfp,
                        (int(*)(void*,char*,int))gzread,
                        (int(*)(void*,const char*,int))gzwrite,
                        (fpos_t(*)(void*,fpos_t,int))gzseek,
                        (int(*)(void*))gzclose);
}
#endif
