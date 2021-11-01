#include "neotags.h"
#include <string.h>

#define mkstring(STR) {(STR), sizeof(STR) - 1, 0}

const struct language_id languages[] = {
    { mkstring("unknown"),     _NONE_   },
    { mkstring("c"),           _C_      },
    { mkstring("cpp"),         _CPP_    },
    { mkstring("objc"),        _C_      },
    { mkstring("objcpp"),      _CPP_    },
    { mkstring("cs"),          _CSHARP_ },
    { mkstring("go"),          _GO_     },
    { mkstring("java"),        _JAVA_   },
    { mkstring("javascript"),  _JS_     },
    { mkstring("flow"),        _FLOW_   },
    { mkstring("typescript"),  _TS_     },
    { mkstring("lisp"),        _LISP_   },
    { mkstring("perl"),        _PERL_   },
    { mkstring("php"),         _PHP_    },
    { mkstring("python"),      _PYTHON_ },
    { mkstring("ruby"),        _RUBY_   },
    { mkstring("rust"),        _RUST_   },
    { mkstring("sh"),          _SH_     },
    { mkstring("vim"),         _VIM_    },
    { mkstring("zsh"),         _ZSH_    },
    { mkstring("vue"),         _VUE_    },
    { mkstring("swift"),       _SWIFT_  },
    { mkstring("lua"),         _LUA_    },
    { mkstring("moon"),        _MOON_   },
    { mkstring("haxe"),        _HAXE_   },
    { mkstring("zig"),         _ZIG_    },
};


const struct language_id *
id_lang(const string *lang)
{
    for (size_t i = 1; i < ARRSIZ(languages); ++i) {
        if (string_eq(lang, &languages[i].lang)) {
            printf("Recognized ft as language \"%s\".", languages[i].lang.s);
            return &languages[i];
        }
    }

    warnx("Language %s not recognized", lang->s);
    return 0;
}


#define DATA (list->lst)
#define PRINT(IT) (printf("%s#%c\t%s\n", ft, DATA[IT]->kind, DATA[IT]->s))

void print_tags_vim(struct StringLst *list, const char *ft)
{
    char *tmp;

    /* Always print the first tag. */
    if (DATA[0]->kind == 'f' && (tmp = strchr(DATA[0]->s, ':'))) {
        printf("%s#%c\t%s\n", ft, DATA[0]->kind, tmp + 1);
    } else {
        printf("%s#%c\t%s\n", ft, DATA[0]->kind, DATA[0]->s);
    }


    for (unsigned i = 1; i < list->num; ++i)
        if (DATA[i]->len != DATA[i - 1]->len ||
            memcmp(DATA[i]->s, DATA[i - 1]->s, DATA[i]->len) != 0) {
            if (DATA[0]->kind == 'f' && (tmp = strchr(DATA[i]->s, ':'))) {
                printf("%s#%c\t%s\n", ft, DATA[i]->kind, tmp + 1);
            } else {
                printf("%s#%c\t%s\n", ft, DATA[i]->kind, DATA[i]->s);
            }
        }
}

#undef DATA
