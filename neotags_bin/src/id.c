#include "neotags.h"

#define mkstring(STR) {(STR), sizeof(STR) - 1, 0}

static const struct language_id {
        const string lang;
        const enum lang_e id;
} languages[] = {
    { mkstring("c"),           _C_      },
    { mkstring("cpp"),         _CPP_    },
    { mkstring("cs"),          _CSHARP_ },
    { mkstring("go"),          _GO_     },
    { mkstring("java"),        _JAVA_   },
    { mkstring("javascript"),  _JS_     },
    { mkstring("lisp"),        _LISP_   },
    { mkstring("perl"),        _PERL_   },
    { mkstring("php"),         _PHP_    },
    { mkstring("python"),      _PYTHON_ },
    { mkstring("ruby"),        _RUBY_   },
    { mkstring("rust"),        _RUST_   },
    { mkstring("sh"),          _SH_     },
    { mkstring("vim"),         _VIM_    },
    { mkstring("zsh"),         _ZSH_    },
};


enum lang_e
id_lang(const string *lang)
{
        for (size_t i = 0; i < ARRSIZ(languages); ++i)
                if (string_eq(lang, &languages[i].lang))
                        return languages[i].id;
        warnx("Language %s not recognized", lang->s);
        return _NONE_;
}


#define DATA (list->lst)
#define PRINT(IT) (printf("%s#%c\t%s\n", ft, DATA[IT]->kind, DATA[IT]->s))

void print_tags_vim(struct StringLst *list, const char *ft)
{
        char *tmp;
        /* eprintf("Looking for vim stuff\n"); */

        /* Always print the first tag. */
        if (DATA[0]->kind == 'f' && (tmp = strchr(DATA[0]->s, ':'))) {
                printf("%s#%c\t%s\n", ft, DATA[0]->kind, tmp + 1);
                /* eprintf("%c - %s\n", DATA[0]->kind, tmp + 1); */
        } else {
                printf("%s#%c\t%s\n", ft, DATA[0]->kind, DATA[0]->s);
                /* eprintf("%c - %s\n", DATA[0]->kind, DATA[0]->s); */
        }


        for (unsigned i = 1; i < list->num; ++i)
                if (DATA[i]->len != DATA[i-1]->len || 
                    memcmp(DATA[i]->s, DATA[i-1]->s, DATA[i]->len) != 0)
                {
                        if (DATA[0]->kind == 'f' && (tmp = strchr(DATA[i]->s, ':'))) {
                                printf("%s#%c\t%s\n", ft, DATA[i]->kind, tmp + 1);
                                /* eprintf("%c - %s\n", DATA[i]->kind, tmp + 1); */
                        } else {
                                printf("%s#%c\t%s\n", ft, DATA[i]->kind, DATA[i]->s);
                                /* eprintf("%c - %s\n", DATA[i]->kind, DATA[i]->s); */
                        }
                }
}

#undef DATA
