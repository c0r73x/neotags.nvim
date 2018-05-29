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


struct lldata {
        char *s;
        char kind;
        size_t len;
};

struct datalist {
        struct lldata **data;
        int64_t num;
        int64_t max;
};


char *program_name;
char *backup_pointers[NUM_BACKUPS];
int backup_iterator;

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
    extern char * basename(char *path);
#else
#   include <unistd.h>
#   define PATHSEP '/'
#endif

#define streq(SA, SB)    (strcmp((SA), (SB)) == 0)
#define strneq(SA, SB)   (strcmp((SA), (SB)) != 0)
#define strCeq(SA, SB)   (strcasecmp((SA), (SB)) == 0)
#define strCneq(SA, SB)  (strcasecmp((SA), (SB)) != 0)

#define modulo(A, B)     (((A) % (B) + (B)) % (B))
#define stringify(VAR_)  #VAR_
#define nputs(STR_)      fputs((STR_), stdout)

#ifdef HAVE_ERR
#   include <err.h>
#   define handle_progname(VAR_) VAR_
#else
    void _warn(bool print_err, const char *fmt, ...) __attribute__((__format__(printf, 2, 3)));
#   define handle_progname(VAR_) basename(VAR_)
#   define warn(...)       _warn(true, __VA_ARGS__)
#   define warnx(...)      _warn(false, __VA_ARGS__)
#   define err(EVAL, ...)  _warn(true, __VA_ARGS__), exit(EVAL)
#   define errx(EVAL, ...) _warn(false, __VA_ARGS__), exit(EVAL)
#endif
#define eputs(STR_)  fputs((STR_), stderr)
#define eprintf(...) fprintf(stderr, __VA_ARGS__)


/*===========================================================================*/


/*
 * utility.c
 */
#define xatoi(STR)      __xatoi((STR), false)
#define s_xatoi(STR)    __xatoi((STR), true)
#define my_getline(PTR) my_fgetline((PTR), stdin)
#define dump_list(LST_) __dump_list((LST_), stderr, #LST_)
#define free_all(...)   __free_all(__VA_ARGS__, NULL)
#define dumpstr(STR_, FNAME_, FP_) __dump_string((STR_), (FNAME_), (FP_), (#STR_))

extern int64_t __xatoi      (char *str, bool strict);
extern size_t  my_fgetline  (char **ptr, void *fp);
extern int     my_fgetc     (void *fp);
extern void *  xmalloc      (const size_t size)                __attribute__((__warn_unused_result__)) __attribute__((__malloc__));
extern void *  xcalloc      (const int num, const size_t size) __attribute__((__warn_unused_result__)) __attribute__((__malloc__));
extern void *  xrealloc     (void *ptr, const size_t size)     __attribute__((__warn_unused_result__));
extern FILE *  safe_fopen   (const char * const __restrict filename, const char * const __restrict mode);
extern bool    file_is_reg  (const char *filename);
extern void  __dump_list    (char **list, FILE *fp, const char *varname);
extern void  __dump_string  (char *str, const char *filename, FILE *fp, const char *varname);
extern void  __free_all     (void *ptr, ...);
extern int   find_num_cpus  (void);


/* 
 * Else
 */
extern int  getlines(struct datalist *tags, const char *comptype, const char *filename);
extern void strip_comments(struct lldata *buffer, const char *lang);


#ifdef __cplusplus
    }
#endif
/*===========================================================================*/
#endif /* neotags.h */
