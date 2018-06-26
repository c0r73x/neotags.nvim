#include "neotags.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_STRINGS 8192
#define DECLARE(FUNC_NAME) \
        static bool FUNC_NAME (const char ch, const bool first)

typedef bool (*cmp_f)(char, bool);

DECLARE(c_func);
DECLARE(vim_func);

static void do_tokenize(struct StringLst *list, char *vimbuf, cmp_f check);
static void tokenize_vim(struct StringLst *list, char *vimbuf, cmp_f check);
static char *strsep_f(char **stringp, cmp_f check);


struct StringLst *
tokenize(struct String *vimbuf)
{
        char *cpy = malloc(vimbuf->len + 1);
        memcpy(cpy, vimbuf->s, vimbuf->len);
        cpy[vimbuf->len] = '\0';
        add_backup(&backup_pointers, cpy);

        struct StringLst *list = malloc(sizeof *list);
        *list = (struct StringLst){
            .lst = nmalloc(INIT_STRINGS, sizeof(struct String *)),
            .num = 0,
            .max = INIT_STRINGS
        };

#if 0
        const struct cmp_funcs *f = &cmp_functions[0];

        for (size_t i = 1, size = ARRSIZ(cmp_functions); i < size; ++i) {
                if (streq(lang, cmp_functions[i].lang)) {
                        f = &cmp_functions[i];
                        break;
                }
        }

#endif
        switch (lang_id->id) {
        case _VIM_:  tokenize_vim(list, cpy, vim_func); break;
        default:     do_tokenize(list, cpy, c_func);  break;
        }
        /* do_tokenize(list, cpy, &cmp_functions[0]); */

        return list;
}


static void
do_tokenize(struct StringLst *list, char *vimbuf, cmp_f check)
{
        char *tok = NULL;

        while ((tok = strsep_f(&vimbuf, check)) != NULL) {
               if (!tok[0])
                       continue;
               struct String *tmp;
               tmp = malloc(sizeof *tmp);
               tmp->s = tok;
               tmp->len = vimbuf - tok - 1;
               add_to_list(list, tmp);
        }
}


static char *
strsep_f(char **stringp, cmp_f check)
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


static bool c_func(const char ch, const bool first)
{
        return (first) ? (ch == '_' || isalpha(ch))
                       : (ch == '_' || isalnum(ch));
} 

static bool vim_func(const char ch, const bool first)
{
        return (first) ? (ch == '_' || isalpha(ch))
                       : (ch == '_' || ch == ':' || isalnum(ch));
} 


//===============================================================================


static void
tokenize_vim(struct StringLst *list, char *vimbuf, cmp_f check)
{
        char *tok = NULL, *col = NULL;

        while ((tok = strsep_f(&vimbuf, check)) != NULL) {
               if (!tok[0])
                       continue;
               struct String *tmp = malloc(sizeof *tmp);
               *tmp = (struct String){ tok, vimbuf - tok - 1LLU, 0 };
               add_to_list(list, tmp);

               if ((col = strchr(tok, ':'))) {
                       tmp = malloc(sizeof *tmp);
                       *tmp = (struct String){ tok, vimbuf - (col + 1) - 1LLU, 0 };
                       add_to_list(list, tmp);
               }
        }
}
