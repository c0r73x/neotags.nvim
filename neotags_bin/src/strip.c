#include "neotags.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

enum basic_types { C_LIKE, PYTHON };

static const struct lang_s {
    const enum lang_e id;
    const enum basic_types type;
} lang_comment_groups[] = {
    { _C_,      C_LIKE, },
    { _CPP_,    C_LIKE, },
    { _CSHARP_, C_LIKE, },
    { _GO_,     C_LIKE, },
    { _JAVA_,   C_LIKE, },
    { _RUST_,   C_LIKE, },
    { _JS_,     C_LIKE, },
    { _TS_,     C_LIKE, },
    { _FLOW_,   C_LIKE, },
    { _VUE_,    C_LIKE, },
    { _SWIFT_,  C_LIKE, },
    { _PYTHON_, PYTHON, },
};

static const struct comment_s {
    const int type;
    const char delim;
} comments[] = {{0, '\0'}, {1, '#'}, {2, ';'}, {3, '"'}};


static void handle_cstyle(struct String *vim_buf);
static void handle_python(struct String *vim_buf);

/*  These functions perform a rather crude stripping of comments and string
    literals from the a few languages. This means fewer words to search though
    when the buffer is searched for applicable tags later on, and avoids any
    false positives caused by tag names appearing in comments and strings. */

void
strip_comments(struct String *buffer)
{
    const struct comment_s *com = NULL;

    for (size_t i = 0; i < ARRSIZ(lang_comment_groups); ++i) {
        if (lang_id->id == lang_comment_groups[i].id) {
            com = &comments[lang_comment_groups[i].type];
            break;
        }
    }

    if (!com) {
        warnx("Comment stripping not supported for %d \"%s\".",
              lang_id->id, lang_id->lang.s);
        return;
    }

    switch (com->type) {
        case C_LIKE:
            handle_cstyle(buffer);
            break;

        case PYTHON:
            handle_python(buffer);
            break;

        default:
            errx(1, "This shouldn't be reachable...");
    }
}


/*============================================================================*/
/* C style languages */

#define QUOTE() (single_q || double_q)

#define check_quote(CHECK, OTHER)                      \
    do {                                               \
        if (!(OTHER)) {                            \
            if (CHECK) {                       \
                if (!escape)               \
                    (CHECK) = false,   \
                              skip = true;       \
            } else                             \
                (CHECK) = true;            \
        }                                          \
        slash = false;                             \
    } while (0)

#define SLS(STR_) (STR_), (sizeof(STR_) - 1)

enum c_com_type { NONE, BLOCK, LINE };


static void
handle_cstyle(struct String *vim_buf)
{
    /* enum c_com_type comment = NONE; */
    bool double_q, single_q, slash, escape, skip, header;
    char *buf, *buf_orig;
    uint32_t   space = 0;
    const char *pos  = vim_buf->s;

    double_q = single_q = slash = escape = skip = header = false;
    buf_orig = buf = malloc(vim_buf->len + 2);

    if (!*pos) {
        errx(1, "Empty vim buffer!");
    }

    /*  Add a non-offensive character to the buffer so we never have to worry
        about going out of bounds when checking 1 character backwards. */
    *buf++ = ' ';

    do {
        switch (*pos) {
            case '/':
                if (!QUOTE() && slash) {
                    const char *tmp = pos + 1;
                    --buf;

                    /*  Find the end of comment, but keep in mind
                        that 'single line' C comments can be multiple
                        lines long if the newline is escaped. */
                    do {
                        tmp = strchr(tmp + 1, '\n');
                    } while (tmp && *(tmp - 1) == '\\');

                    if (!tmp) {
                        warnx("Couldn't find end of comment.");
                        break;
                    }

                    pos = tmp;

                    /*  Add the newline only if the last char in the
                        output buffer was not also a newline. */
                    if (*(buf - 1) == '\n') {
                        skip = true;
                    }
                } else if (!QUOTE()) {
                    slash = true;
                }

                break;

            case '*':
                if (!QUOTE() && slash) {
                    const char *tmp;
                    --buf;

                    if (!(tmp = strstr(pos, "*/"))) {
                        errx(1, "Couldn't find end of comment.");
                    }

                    pos = tmp + 2;

                    /* Don't add newlines after infixed comments. */
                    if (*pos == '\n' && *(buf - 1) == '\n') {
                        skip = true;
                    }

                    slash = false;
                }

                break;

            case '\n':
                if (!escape) {
                    slash = double_q = false;

                    if (*(buf - 1) == '\n') {
                        skip = true;
                    }

                    header = false;
                }

                break;

            case '#':
                ;
                /* Strip out include directives as well. */
                const char *endln;

                if (*(buf - 1) == '\n' && (endln = strchr(pos, '\n'))) {
                    const char *tmp = pos + 1;

                    while (tmp < endln && isblank(*tmp)) {
                        ++tmp;
                    }

                    if (strncmp(tmp, SLS("include")) == 0) {
                        header = true;
                        pos = endln - 1;
                        continue;
                    }
                }

                slash = false;
                break;

            case '\\':
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

        escape = (*pos == '\\') ? !escape : false;
        /*  Avoid adding spaces at the start of lines, and don't add more
            than one space or newline in succession. */
        space  = (isblank(*pos) &&
                  !(skip = (skip) ? true : * (buf - 1) == '\n')
                 ) ? space + 1 : 0;

        if (skip) {
            skip = false;
        } else if (!QUOTE() && !header && space < 2) {
            *buf++ = *pos;
        }

    } while (*pos++);

    *buf = '\0';

    free(vim_buf->s);
    vim_buf->len = buf - buf_orig - 1;
    vim_buf->s   = xrealloc(buf_orig, vim_buf->len + 1);
}

#undef QUOTE
#undef check_quote


/*============================================================================*/
/* Python */


#define QUOTE() (Single.Q || Double.Q || in_docstring)

#define check_docstring(AA, BB)                                      \
    do {                                                             \
        if (in_docstring) {                                      \
            if ((AA).cnt == 3)                               \
                --(AA).cnt;                              \
            else if (*(pos - 1) == (AA).ch)                  \
                --(AA).cnt;                              \
            else                                             \
                (AA).cnt = 3;                            \
            \
            in_docstring = ((AA).cnt != 0) ? (AA).val        \
                           : NO_DOCSTRING;   \
            if (!in_docstring)                               \
                skip = true;                             \
        } else {                                                 \
            if ((AA).cnt == 0 && !((AA).Q || (BB).Q))        \
                ++(AA).cnt;                              \
            else if (*(pos - 1) == (AA).ch)                  \
                ++(AA).cnt;                              \
            \
            in_docstring = ((AA).cnt == 3) ? (AA).val        \
                           : NO_DOCSTRING;   \
            \
            if (in_docstring) {                              \
                (AA).Q = (BB).Q = false;                 \
                skip = true;                             \
            } else if (!(BB).Q && !comment) {                \
                if ((AA).Q) {                            \
                    if (!escape)                     \
                        (AA).Q = false,          \
                                 skip = true;             \
                } else                                   \
                    (AA).Q = true;                   \
            }                                                \
        }                                                        \
    } while (0)


enum docstring_e {
    NO_DOCSTRING,
    SINGLE_DOCSTRING,
    DOUBLE_DOCSTRING
};

struct py_quote {
    bool Q;
    int cnt;
    char ch;
    enum docstring_e val;
};


static void
handle_python(struct String *vim_buf)
{
    enum docstring_e in_docstring = NO_DOCSTRING;
    struct py_quote  Single = { false, 0, '\'', SINGLE_DOCSTRING };
    struct py_quote  Double = { false, 0, '"',  DOUBLE_DOCSTRING };
    const char      *pos    = vim_buf->s;
    uint32_t         space  = 0;

    char *buf, *buf_orig;
    bool escape, comment, skip;

    buf    = buf_orig = malloc(vim_buf->len + 2LLU);
    escape = comment  = skip = false;

    if (*pos == '\0') {
        errx(1, "Empty vim buffer!");
    }

    /*  Add a non-offensive character to the buffer so we never have to worry
        about going out of bounds when checking 1 character backwards. */
    *buf++ = ' ';

    do {
        if (!comment && !QUOTE() && !escape && *pos == '#') {
            comment = true;
            space   = 0;
            continue;
        }

        if (comment && *pos != '\n') {
            continue;
        }

        switch (*pos) {
            case '\n':
                if (*(buf - 1) == '\n') {
                    skip = true;
                }

                comment = false;
                space = 0;
                break;

            case '"':
                if (in_docstring != SINGLE_DOCSTRING) {
                    check_docstring(Double, Single);
                }

                space = 0;
                break;

            case '\'':
                if (in_docstring != DOUBLE_DOCSTRING) {
                    check_docstring(Single, Double);
                }

                space = 0;
                break;

            case '\t':
            case ' ':
                if (*(buf - 1) == '\n') {
                    skip = true;
                } else {
                    ++space;
                }

                break;

            default:
                space = 0;
        }

        /*  If less than 3 of any type of quote appear in a row, reset
            the corresponding counter to 0. */
        switch (in_docstring) {
            case SINGLE_DOCSTRING:
                if (Single.cnt < 3 && *pos != Single.ch) {
                    Single.cnt = 3;
                }

                Double.cnt = 0;
                break;

            case DOUBLE_DOCSTRING:
                if (Double.cnt < 3 && *pos != Double.ch) {
                    Double.cnt = 3;
                }

                Single.cnt = 0;
                break;

            case NO_DOCSTRING:
                if (Single.cnt > 0 && *pos != Single.ch) {
                    Single.cnt = 0;
                }

                if (Double.cnt > 0 && *pos != Double.ch) {
                    Double.cnt = 0;
                }

                break;

            default: /* not reachable */
                abort();
        }

        if (skip) {
            skip = false;
        } else if (!QUOTE() && space < 2) {
            *buf++ = *pos;
        }

        escape = (*pos == '\\' ? !escape : false);

    } while (*pos++);

    *buf = '\0';

    free(vim_buf->s);
    vim_buf->len = buf - buf_orig - 1LLU;
    vim_buf->s   = xrealloc(buf_orig, vim_buf->len + 1);
}
