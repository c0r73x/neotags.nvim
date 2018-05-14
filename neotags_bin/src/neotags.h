#ifndef SRC_NEOTAGS_H
#define SRC_NEOTAGS_H
/*===========================================================================*/
#ifdef __cplusplus
    extern "C" {
#endif
#ifdef HAVE_CONFIG_H
#   include "config.h"
#else  /* This just shuts up linters too lazy to include config.h */
#   if defined(__GNU_LIBRARY__) || defined(__FreeBSD__)
#      define HAVE_ERR
#   endif
#   define VERSION "1.1.0"
#   define PACKAGE_STRING "neotags 1.1.0"
#   define _GNU_SOURCE
#endif
#ifndef HAVE_STRLCPY
#   if defined(HAVE_LIBBSD) && defined(HAVE_BSD_BSD_H)
#      include <bsd/bsd.h>
#   else
#      include "bsd_funcs.h"
#   endif
#endif
#ifdef HAVE_MALLOPT
#   include <malloc.h>
#endif
#ifdef _MSC_VER
#   define _CRT_SECURE_NO_WARNINGS
#   define _CRT_NONSTDC_NO_WARNINGS
#endif
/*===========================================================================*/

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LLTYPE struct string *
#define NUM_POINTERS 20

/*===========================================================================*/

struct string {
        char *s;
        char kind;
        size_t len;
};

struct linked_list {
        struct Node *head;
        struct Node *tail;
        uint32_t size;
        uint8_t can_free_data;
};

struct Node {
        LLTYPE data;
        struct Node *prev;
        struct Node *next;
};

enum ll_pop_type {
        DEL_ONLY,
        RET_ONLY,
        BOTH
};

char *program_name;
char *backup_pointers[NUM_POINTERS];
int backup_iterator;

/*===========================================================================*/

#if (defined(_WIN64) || defined(_WIN32)) && !defined(__CYGWIN__)
#   include <io.h>
#   define __attribute__(...)
#   define strcasecmp  _stricmp
#   define strncasecmp _strnicmp
#   define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   undef BUFSIZ
#   define BUFSIZ 8192
    char * basename(char *path);
#else
#   include <unistd.h>
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
    void _warn(bool print_err, const char *fmt, ...);
#   define handle_progname(VAR_) basename(VAR_)
/* #   ifdef DEBUG */
#      define warn(...)    _warn(true, __VA_ARGS__)
#      define warnx(...)   _warn(false, __VA_ARGS__)
/* #   endif */
#   define err(EVAL, ...)  _warn(true, __VA_ARGS__), exit(EVAL)
#   define errx(EVAL, ...) _warn(false, __VA_ARGS__), exit(EVAL)
#endif
/* #ifdef DEBUG */
#   define eputs(STR_)    fputs((STR_), stderr)
#   define eprintf(...)   fprintf(stderr, __VA_ARGS__)
/* #else
#   define warn(...)    ;
#   define warnx(...)   ;
#   define eputs(...)   ;
#   define eprintf(...) ;
#endif */


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

int64_t __xatoi      (char *str, bool strict);
size_t  my_fgetline  (char **ptr, void *fp);
int     my_fgetc     (void *fp);
void *  xmalloc      (const size_t size)                __attribute__((malloc));
void *  xcalloc      (const int num, const size_t size) __attribute__((malloc));
void *  xrealloc     (void *ptr, const size_t size)     __attribute__((malloc));
FILE *  safe_fopen   (const char * const restrict filename, const char * const restrict mode);
bool    file_is_reg  (const char *filename);
void  __dump_list    (char **list, FILE *fp, const char *varname);
void  __dump_string  (char *str, const char *filename, FILE *fp, const char *varname);
void  __free_all     (void *ptr, ...);

struct linked_list * get_all_lines(const char *filename);
struct linked_list * llstrsep(struct string *buffer);


/*
 * linked_list.c
 */
#define ll_pop(LIST)        _ll_popat((LIST), -1, BOTH)
#define ll_dequeue(LIST)    _ll_popat((LIST), 0, BOTH)
#define ll_pop_at(LIST,IND) _ll_popat((LIST), (IND), BOTH)
#define ll_get(LIST,IND)    _ll_popat((LIST), (IND), RET_ONLY)
#define ll_remove(LIST,IND) _ll_popat((LIST), (IND), DEL_ONLY)

struct linked_list * new_list(uint8_t can_free_data);

void   ll_add(struct linked_list *list, LLTYPE data);
void   ll_append(struct linked_list *list, LLTYPE data);
LLTYPE _ll_popat(struct linked_list *list, long index, enum ll_pop_type type);
bool   ll_find_str(struct linked_list *list, char *str);
void   destroy_list(struct linked_list *list);


void getlines(struct linked_list *ll, const char *comptype, const char *filename);
char * strip_comments(struct string *buffer, const char *lang);


#ifdef __cplusplus
    }
#endif
/*===========================================================================*/
#endif /* neotags.h */
