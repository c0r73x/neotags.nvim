#ifndef SRC_NEOTAGS_H
#define SRC_NEOTAGS_H
/*===========================================================================*/
#ifdef __cplusplus
extern "C" {
#endif
#ifdef _MSC_VER /* Microsoft sure likes to complain... */
#   pragma warning(disable : 4996)
#   define _CRT_SECURE_NO_WARNINGS
#   define _CRT_NONSTDC_NO_WARNINGS
#   define __attribute__(...)
#endif
#ifdef HAVE_CONFIG_H
#   include "config.h"
#else  /* This just shuts up linters too lazy to include config.h */
#   if defined(__GNUC__) || defined(__FreeBSD__)
#      define USE_PTHREADS
#      define HAVE_ERR
#   endif
#   define VERSION "1.1.0"
#   define PACKAGE_STRING "neotags 1.1.0"
#   define _GNU_SOURCE
#endif
#ifndef HAVE_STRLCPY
#   if defined(HAVE_LIBBSD) && defined(HAVE_BSD_BSD_H) && 0
#      include <bsd/bsd.h>
#   else
#      include "bsd_funcs.h"
#   endif
#endif
#ifdef HAVE_MALLOPT
#   include <malloc.h>
#endif
/*===========================================================================*/

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define PROG_ID "C"
#define NUM_BACKUPS 256
#define INIT_TAGS 1024
#define TAGS_INC  512

/*===========================================================================*/


struct String {
    char *s;
    size_t len;
    char kind;
};

struct StringLst {
    struct String **lst;
    int64_t num;
    int64_t max;
};

struct Backups {
    char **lst;
    int64_t num;
    int64_t max;
};

typedef struct String string;
typedef struct StringLst strlist;

enum lang_e {
    _NONE_,
    _C_,
    _CPP_,
    _CSHARP_,
    _GO_,
    _JAVA_,
    _JS_,
    _TS_,
    _FLOW_,
    _LISP_,
    _PERL_,
    _PHP_,
    _PYTHON_,
    _RUBY_,
    _RUST_,
    _SH_,
    _VIM_,
    _ZSH_,
    _VUE_,
};

extern const struct language_id {
    const struct String lang;
    const enum lang_e id;
} languages[];

const struct language_id *lang_id;
char *program_name;
struct Backups backup_pointers;

/*===========================================================================*/

#if (defined(_WIN64) || defined(_WIN32)) && !defined(__CYGWIN__)
#   define DOSISH
#   include <io.h>
#   include <Windows.h>
#   define strcasecmp  _stricmp
#   define strncasecmp _strnicmp
#   define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   undef BUFSIZ
#   define BUFSIZ 8192
#   define PATHSEP '\\'
extern char *basename(char *path);
#else
#   include <unistd.h>
#   define PATHSEP '/'
#endif

#define streq(SA, SB)      (strcmp((SA), (SB)) == 0)
#define strCeq(SA, SB)     (strcasecmp((SA), (SB)) == 0)
#define memeq(MA, MB, SIZ) (memcmp((MA), (MB), (SIZ)) == 0)
#define ARRSIZ(ARR_)       (sizeof(ARR_) / sizeof(*(ARR_)))

#define modulo(A, B)     (((A) % (B) + (B)) % (B))
#define stringify(VAR_)  #VAR_
#define nputs(STR_)      fputs((STR_), stdout)

#ifdef HAVE_ERR
#   include <err.h>
#   define handle_progname(VAR_) VAR_
#else
void _warn(bool print_err, const char *fmt,
           ...) __attribute__((__format__(printf, 2, 3)));
#   define handle_progname(VAR_) basename(VAR_)
#   define warn(...)       _warn(true, __VA_ARGS__)
#   define warnx(...)      _warn(false, __VA_ARGS__)
#   define err(EVAL, ...)  _warn(true, __VA_ARGS__), exit(EVAL)
#   define errx(EVAL, ...) _warn(false, __VA_ARGS__), exit(EVAL)
#endif
#define eputs(STR_)  fputs((STR_), stderr)
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define xfree(PTR_) ((PTR_) ? free(PTR_), (PTR_) = NULL : NULL)


/*===========================================================================*/


/*
    utility.c
*/
#define xatoi(STR)      __xatoi((STR), false)
#define s_xatoi(STR)    __xatoi((STR), true)
#define my_getline(PTR) my_fgetline((PTR), stdin)
#define dump_list(LST_) __dump_list((LST_), stderr, #LST_)
#define free_all(...)   __free_all(__VA_ARGS__, NULL)
#define free_all_strlists(...)     __free_all_strlists(__VA_ARGS__, NULL)
#define dumpstr(STR_, FNAME_, FP_) __dump_string((STR_), (FNAME_), (FP_), (#STR_))

extern int64_t __xatoi(char *str, bool strict);
extern size_t  my_fgetline(char **ptr, void *fp);
extern int     my_fgetc(void *fp);
extern void    add_to_list(strlist *list, string *str);
extern void    add_backup(struct Backups *list, void *str);
extern void   *xmalloc(const size_t size)                __attribute__((
            warn_unused_result)) __attribute__((malloc));
extern void   *xcalloc(const int num,
                       const size_t size) __attribute__((warn_unused_result)) __attribute__((
                                   malloc));
extern void   *xrealloc(void *ptr,
                        const size_t size)     __attribute__((warn_unused_result));
extern FILE   *safe_fopen(const char *const __restrict filename,
                          const char *const __restrict mode);
extern bool    file_is_reg(const char *filename);
extern void  __dump_list(char **list, FILE *fp, const char *varname);
extern void  __dump_string(char *str, const char *filename, FILE *fp,
                           const char *varname);
extern void  __free_all(void *ptr, ...);
extern void  __free_all_strlists(strlist *ptr, ...);
extern int   find_num_cpus(void);
#ifdef HAVE_REALLOCARRAY
extern void *xreallocarray(void *ptr, size_t num,
                           size_t size) __attribute__((__warn_unused_result__));
#  define nmalloc(NUM_, SIZ_)        reallocarray(NULL, (NUM_), (SIZ_))
#  define nrealloc(PTR_, NUM_, SIZ_) xreallocarray((PTR_), (NUM_), (SIZ_))
#else
#  define nmalloc(NUM_, SIZ_)        malloc(((size_t)(NUM_)) * ((size_t)(SIZ_)))
#  define nrealloc(PTR_, NUM_, SIZ_) xrealloc((PTR_), ((size_t)(NUM_)) * ((size_t)(SIZ_)))
#endif


/*
    Else
*/
extern int  getlines(strlist *tags, const char *comptype,
                     const char *filename);
extern void strip_comments(string *buffer);
extern strlist *tokenize(string *vimbuf);
/* extern enum lang_e id_lang(const string *lang); */
extern const struct language_id *id_lang(const string *lang);
extern void print_tags_vim(strlist *list, const char *ft);


/*===========================================================================*/


#define ntostring(STR, LEN) (string){ (STR), (size_t)(LEN), 0 }

#define string_eq(s1, s2) \
    (((s1)->len == (s2)->len) && memeq((s1)->s, (s2)->s, (s1)->len))
/* (((s1)->len == (s2)->len) && (memcmp((s1)->s, (s2)->s, (s1)->len) == 0)) */

#define string_eq_i(s1, s2) \
    (((s1)->len == (s2)->len) && strCeq((s1)->s, (s2)->s))
/* (((s1)->len == (s2)->len) && (strcasecmp((s1)->s, (s2)->s)) == 0) */

#define string_lit_eq(STR, CSTR) \
    string_eq(STR, ((string[]){{ (char *)(CSTR), (sizeof(CSTR) - 1), 0 }}))

#define string_lit_eq_i(STR, CSTR) \
    string_eq_i(STR, ((string[]){{ (char *)(CSTR), (sizeof(CSTR) - 1), 0 }}))

#ifdef __GNUC__
#  define tostring(STR) ({char *s = (STR); (string){ s, strlen(s), 0 };})
#else
static inline string tostring(char *cstr)
{
    return (string) {
        cstr, strlen(cstr), 0
    };
}
#endif

#if 0
static inline bool
__string_lit_eq(const string *str1, const char *const str2, size_t size)
{
    string tmp = { (char *)str2, size, 0 };
    /* warnx("Comparing %s to lit %s (size %zu)", str1->s, (&tmp)->s, (&tmp)->len); */
    return string_eq(str1, &tmp);
}

static inline bool
__string_lit_eq_i(const string *str1, const char *const str2, size_t size)
{
    string tmp = { (char *)str2, size, 0 };
    /* warnx("Comparing %s to lit %s (size %zu)", str1->s, (&tmp)->s, (&tmp)->len); */
    return string_eq_i(str1, &tmp);
}

#define string_lit_eq(STR, CSTR) \
    __string_lit_eq((STR), (CSTR), (sizeof(CSTR) - 1))
#define string_lit_eq_i(STR, CSTR) \
    __string_lit_eq_i((STR), (CSTR), (sizeof(CSTR) - 1))
#endif


/*===========================================================================*/

#ifdef __cplusplus
}
#endif
/*===========================================================================*/
#endif /* neotags.h */
