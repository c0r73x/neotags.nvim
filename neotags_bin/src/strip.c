#include "neotags.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define CC(VAR_)     ((const struct lldata *const)(VAR_))
#define ARRSIZ(ARR_) (sizeof(ARR_) / sizeof(*(ARR_)))

enum lang_e {
        _C_,
        _CPP_,
        _CSHARP_,
        _GO_,
        _JAVA_,
        _PYTHON_,
};

enum basic_types { C_LIKE, PYTHON };

static const struct lang_s {
        const char *lang;
        const enum basic_types type;
        const enum lang_e lang_id;
} languages[] = {
    { "c",      C_LIKE, _C_      },
    { "cpp",    C_LIKE, _CPP_    },
    { "cs",     C_LIKE, _CSHARP_ },
    { "go",     C_LIKE, _GO_     },
    { "java",   C_LIKE, _JAVA_   },
    { "python", PYTHON, _PYTHON_ },
};

static const struct comment_s {
        const int type;
        const char delim;
} comments[] = {{0, '\0'}, {1, '#'}, {2, ';'}, {3, '"'}};


static struct lldata *handle_cstyle(const struct lldata *const vim_buf);
static struct lldata *handle_python(const struct lldata *const vim_buf, const char delim);


struct lldata *
strip_comments(struct lldata *buffer, const char *lang)
{
        size_t i, size;

        for (i = 0, size = ARRSIZ(languages); i < size; ++i)
                if (streq(lang, languages[i].lang))
                        break;

        if (i == size) {
                warnx("Failed to identify language '%s'.", lang);
                return NULL;
        }

        const struct comment_s com = comments[languages[i].type];
        struct lldata *ret = NULL;

        if (com.type == C_LIKE)
                ret = handle_cstyle(CC(buffer));
        else if (com.type == PYTHON)
                ret = handle_python(CC(buffer), com.delim);
        else
                errx(1, "This shouldn't be reachable...");

        return ret;
}


/*============================================================================*/
/* C style languages */

#define QUOTE (single_q || double_q)

#define check_quote(CHECK, OTHER)                                       \
    do {                                                                \
            if (!(OTHER) && !comment) {                                 \
                    if (CHECK) {                                        \
                            if (!escape)                                \
                                    (CHECK) = false, transition = true; \
                    } else {                                            \
                            (CHECK) = true;                             \
                    }                                                   \
            }                                                           \
    } while (0)

#define setcomment(TYPE) comment = (TYPE), buf = marker
#define set_slash()      slash = true, marker = buf

enum c_com_type {
        NONE,
        BLOCK,
        LINE
};


static struct lldata *
handle_cstyle(const struct lldata *const vim_buf)
{
        enum c_com_type comment = NONE;
        bool double_q, single_q, slash, escape, transition;
        char *buf, *buf_orig, *marker;
        uint32_t space  = 0;
        const char *pos = vim_buf->s;

        double_q = single_q = slash  = escape = transition = false;
        buf_orig = marker   = buf    = xmalloc((size_t)vim_buf->len);

        if (!*pos)
                errx(1, "Empty vim buffer!");

        do {
                switch (*pos) {
                case '\\':
                        break;

                case '/':
                        if (comment == BLOCK && *(pos - 1) == '*') {
                                comment    = NONE;
                                slash      = false;
                                transition = true;
                        } else if (!double_q) {
                                if (slash && !comment)
                                        setcomment(LINE);
                                else
                                        set_slash();
                        }
                        break;

                case '*':
                        if (!double_q && slash) {
                                if (!comment)
                                        setcomment(BLOCK);
                                slash = false;
                        }
                        break;

                case '\n':
                        if (!escape) {
                                slash = double_q = false;
                                if (comment == LINE)
                                        comment = NONE;
                        }
                        break;

                case '"':
                        check_quote(double_q, single_q);
                        break;

                case '\'':
                        check_quote(single_q, double_q);
                        break;

                default:
                        slash = false;
                }

                escape = (*pos == '\\' ? (escape ? false : true) : false);
                space  = ((isblank(*pos)) ? space + 1 : 0);

                if (transition)
                        transition = false;
                else if (!comment && !QUOTE && space < 2)
                        *buf++ = *pos;

        } while (*pos++);

        *buf++ = '\0';
        struct lldata *ret = xmalloc(buf - buf_orig);
        *ret = (struct lldata){ xrealloc(buf_orig, buf - buf_orig), '\0',
                                buf - buf_orig };

        return ret;
}

#undef QUOTE
#undef check_quote
#undef setcomment
#undef set_slash


/*============================================================================*/
/* Python */


#define QUOTE (Single.b || Double.b || in_docstring)

#define check_docstring(AA, BB)                                     \
    do {                                                            \
            if (in_docstring) {                                     \
                    if ((AA).cnt == 3)                              \
                            --(AA).cnt;                             \
                    else if (*(pos - 1) == (AA).ch)                 \
                            --(AA).cnt;                             \
                    else                                            \
                            (AA).cnt = 3;                           \
                                                                    \
                    in_docstring = ((AA).cnt != 0) ? (AA).val       \
                                                   : NO_DOCSTRING;  \
                                                                    \
                    if (in_docstring) {                             \
                            (AA).b = (BB).b = false;                \
                            transition = true;                      \
                    }                                               \
                                                                    \
            } else {                                                \
                    if ((AA).cnt == 0 && !((AA).b || (BB).b))       \
                            ++(AA).cnt;                             \
                    else if (*(pos - 1) == (AA).ch)                 \
                            ++(AA).cnt;                             \
                                                                    \
                    in_docstring = ((AA).cnt == 3) ? (AA).val       \
                                                   : NO_DOCSTRING;  \
                                                                    \
                    if (in_docstring) {                             \
                            (AA).b = (BB).b = false;                \
                            transition = true;                      \
                    } else if (!(BB).b && !comment) {               \
                            if ((AA).b) {                           \
                                    if (!escape)                    \
                                            (AA).b = false,         \
                                            transition = true;      \
                            } else {                                \
                                    (AA).b = true;                  \
                            }                                       \
                    }                                               \
            }                                                       \
    } while (0)


enum docstring_e {
        NO_DOCSTRING,
        SINGLE_DOCSTRING,
        DOUBLE_DOCSTRING
};

struct py_quote {
        bool b;
        int cnt;
        char ch;
        enum docstring_e val;
};

#define DUMP(VAR_) (#VAR_), 0, (VAR_)
#define FIELDB "%s: \033[%dm\033[34m%d\033[0m, "
#define FIELDG "%s: \033[%um\033[32m%d\033[0m, "


static struct lldata *
handle_python(const struct lldata *const vim_buf, const char delim)
{
        enum docstring_e in_docstring = NO_DOCSTRING;
        struct py_quote Single = { false, 0, '\'', SINGLE_DOCSTRING };
        struct py_quote Double = { false, 0, '"',  DOUBLE_DOCSTRING };
        const char *pos        = vim_buf->s;
        uint32_t space         = 0;

        char *buf, *buf_orig;
        bool escape, comment, transition;

        buf    = buf_orig = xmalloc((size_t)vim_buf->len);
        escape = comment  = transition = false;

        if (*pos == '\0')
                errx(1, "Empty vim vim_buf!");

        do {
                if (!comment && !QUOTE && !escape && *pos == delim) {
                        comment = true;
                        space   = 0;
                        continue;
                }

                if (comment && *pos != '\n')
                        continue;

                switch (*pos) {
                case '\n':
                        comment = false;
                        space = 0;
                        goto cont;

                case '"':
                        if (in_docstring != SINGLE_DOCSTRING)
                                check_docstring(Double, Single);
                        space = 0;
                        break;

                case '\'':
                        if (in_docstring != DOUBLE_DOCSTRING)
                                check_docstring(Single, Double);
                        space = 0;
                        break;

                case ' ':
                        ++space;
                        break;

                default:
                        space = 0;
                }

                /* If less than 3 of any type of quote appear in a row, reset
                 * the corresponding counter to 0. */
                switch (in_docstring) {
                case SINGLE_DOCSTRING:
                        if (Single.cnt < 3 && *pos != Single.ch)
                                Single.cnt = 3;
                        Double.cnt = 0;
                        break;

                case DOUBLE_DOCSTRING:
                        if (Double.cnt < 3 && *pos != Double.ch)
                                Double.cnt = 3;
                        Single.cnt = 0;
                        break;

                case NO_DOCSTRING:
                        if (Single.cnt > 0 && *pos != Single.ch)
                                Single.cnt = 0;
                        if (Double.cnt > 0 && *pos != Double.ch)
                                Double.cnt = 0;
                        break;

                default:
                        errx(1, "Not reachable.");
                }

                if (transition)
                        transition = false;
                else if (!QUOTE && space < 2)
cont:
                        *buf++ = *pos;

                escape = (*pos == '\\' ? (escape ? false : true) : false);

        } while (*pos++);

        *buf++ = '\0';
        struct lldata *ret = xmalloc(buf - buf_orig);
        *ret = (struct lldata){ xrealloc(buf_orig, buf - buf_orig), '\0',
                                buf - buf_orig };

        return ret;
}
