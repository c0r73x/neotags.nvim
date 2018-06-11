#include "neotags.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_STRINGS 8192
#define FIRST() (ch == '_' || ch == ':' || isalpha(ch))
#define ELSE()  (ch == '_' || ch == ':' || isalnum(ch))

static void do_tokenize(struct StringLst *list, char *vimbuf);
static char * strsep_f(char **stringp, bool (*check)(char, bool));
static bool compar_func(char ch, bool first);



struct StringLst *
tokenize(struct String *vimbuf)
{
        char *cpy = malloc(vimbuf->len + 1);
        memcpy(cpy, vimbuf->s, vimbuf->len);
        cpy[vimbuf->len] = '\0';
        add_backup(&backup_pointers, cpy);

        struct StringLst *list = malloc(sizeof *list);
        *list = (struct StringLst){
                .data = nmalloc(INIT_STRINGS, sizeof(struct String *)),
                .num  = 0,
                .max  = INIT_STRINGS
        };

        do_tokenize(list, cpy);

        return list;
}


static void
do_tokenize(struct StringLst *list, char *vimbuf)
{
        char *tok = NULL;

        while ((tok = strsep_f(&vimbuf, &compar_func)) != NULL) {
               if (!tok[0])
                       continue;
               struct String *tmp;
again:
               tmp = malloc(sizeof *tmp);
               tmp->s = tok;
               tmp->len = vimbuf - tok - 1LLU;
               add_to_list(list, tmp);

               if (tok[0] == '&') {
                       ++tok;
                       goto again;
               }
        }
}


static char *
strsep_f(char **stringp, bool (*check)(char, bool))
{
        char *ptr, *tok;
        if ((ptr = tok = *stringp) == NULL)
                return NULL;

        for (bool first = true;;first = false) {
                const char src_ch = *ptr++;
                if (!check(src_ch, first)) {
                        if (src_ch == '\0')
                                ptr = NULL;
                        else
                                *(ptr - 1) = '\0';
                        *stringp = ptr;
                        return tok;
                }
        }
}


static bool
compar_func(const char ch, const bool first)
{
        bool ret;

        if (first)
                ret = FIRST();
        else
                ret = ELSE();

        return ret;
} 
