#include "neotags.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_STRINGS 8192

static void do_tokenize(struct StringLst *list, char *vimbuf);
/* static void dump_tokens(struct StringLst *list); */
static char * strsep_f(char **stringp, bool (*check)(const char, const bool));

static inline void add_string(struct StringLst *list, struct String *str);
static bool compar_func(const char ch, const bool first);

/* static FILE *LOG_FILE; */
int64_t calls;


struct StringLst *
tokenize(struct String *vimbuf)
{
        char *cpy = malloc(vimbuf->len + 1);
        memcpy(cpy, vimbuf->s, vimbuf->len);
        cpy[vimbuf->len] = '\0';
        backup_pointers[backup_iterator++] = cpy;

        struct StringLst *list = malloc(sizeof *list);
        *list = (struct StringLst){
                .data = nmalloc(INIT_STRINGS, sizeof(struct String *)),
                .num  = 0,
                .max  = INIT_STRINGS
        };

        /* LOG_FILE = safe_fopen("/home/bml/token.log", "wb"); */
        /* fputs(cpy, LOG_FILE); fputs("\n\n\n", LOG_FILE); */
        do_tokenize(list, cpy);
        /* dump_tokens(list); */

        /* fclose(LOG_FILE); */
        warnx("There were %ld calls to realloc.", calls);
        return list;
}


static void
do_tokenize(struct StringLst *list, char *vimbuf)
{
        char *tok = NULL;

        while ((tok = strsep_f(&vimbuf, &compar_func)) != NULL) {
               if (!*tok)
                       continue;
               struct String *tmp = malloc(sizeof *tmp);
               tmp->s = tok;
               tmp->len = vimbuf - tok - 1LLU;
               add_string(list, tmp);
        }
}


#if 0
static void
dump_tokens(struct StringLst *list)
{
        for (int i = 0; i < list->num; ++i) {
                fputs(list->data[i]->s, LOG_FILE);
                fputc('\n', LOG_FILE);
        }
}
#endif


static char *
strsep_f(char **stringp, bool (*check)(const char, const bool))
{
        char *ptr, *tok;
        if ((ptr = tok = *stringp) == NULL)
                return NULL;

        for (bool first = true;;first = false) {
                const char src_ch = *ptr++;
                if (check(src_ch, first)) {
                        if (src_ch == '\0')
                                ptr = NULL;
                        else
                                *(ptr - 1) = '\0';
                        *stringp = ptr;
                        return tok;
                }
        }
}


static inline void
add_string(struct StringLst *list, struct String *str)
{
        if (list->num == (list->max - 1)) {
                list->data = nrealloc(list->data, (list->max <<= 2),
                                      sizeof *list->data);
                ++calls;
        }
        list->data[list->num++] = str;
}


static bool
compar_func(const char ch, const bool first)
{
        bool ret;

        if (first)
                ret = (ch != '_' && ch != ':' && !isalpha(ch));
        else
                ret = (ch != '_' && ch != ':' && !isalnum(ch));

        return ret;
}
