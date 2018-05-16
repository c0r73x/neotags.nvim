#include "neotags.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct linked_list *search(
        const struct linked_list *taglist, const char *lang,
        const char *order, const char *const *skip, const char *const *equiv
);

static char **get_colon_data  (char *oarg);
static inline void print_data (const struct linked_list *ll, const char *vim_buf);

#ifdef HAVE_STRDUPA
#   define STRDUP strdupa
#else
#   define STRDUP strdup
#endif
#define REQUIRED_INPUT 8
#define CCC(ARG_) ((const char *const *)(ARG_))
#define CSS(NODE_) ((struct string *)(NODE_))


int
main(int argc, char **argv)
{
        program_name = handle_progname(*argv++);
        if (isatty(0))
                errx(1, "This program can't be run manually.");
        if (--argc != REQUIRED_INPUT)
                errx(2, "Error: Wrong number of paramaters (%d, need %d).",
                     argc, REQUIRED_INPUT);

        char **files   = get_colon_data(*argv++);
        char *ctlang   = *argv++;
        char *vimlang  = *argv++;
        char *order    = *argv++;
        bool strip_com = xatoi(*argv++);
        int64_t nchars = xatoi(*argv++);
        char *vim_buf  = xmalloc(nchars + 1);
        char **skip    = get_colon_data(*argv++);
        char **equiv   = get_colon_data(*argv++);

        struct linked_list *taglist = new_list(false);
        warnx("ctlang: %s, vimlang: %s\n", ctlang, vimlang);
        dump_list(files);
        dump_list(equiv);

        for (char **ptr = files; *ptr != NULL; ptr += 2)
                getlines(taglist, *ptr, *(ptr + 1));

        fread(vim_buf, 1, nchars, stdin);
        vim_buf[nchars] = '\0';

        if (strip_com) {
                warnx("Stripping comments...\n");
                struct string tmp = {vim_buf, '\0', nchars + 1};
                char *buf = strip_comments(&tmp, vimlang);
                if (buf) {
                        free(vim_buf);
                        vim_buf = buf;
                }
        }

        struct linked_list *ll = search(taglist, ctlang, order,
                                        CCC(skip), CCC(equiv));
        if (ll) {
                print_data(ll, vim_buf);
                destroy_list(ll);
        }

        /* pointlessly free everything */
        for (int i = 0; i < backup_iterator; ++i)
                free(backup_pointers[i]);
        destroy_list(taglist);
        free_all(files, skip, equiv, vim_buf);

        return 0;
}


#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
#   define SEPCHAR ';'
#else
#   define SEPCHAR ':'
#endif
static char **
get_colon_data(char *oarg)
{
        int num = 0;
        char *arg = oarg;

        if (*arg != '\0')
                do if (*arg == SEPCHAR) {
                        *arg++ = '\0';
                        ++num;
                } while (*arg++);

        /* The loop above will miss the last element, so we increment num. */
        char **data = xmalloc(sizeof(*data) * ++num);
        arg = oarg;

        for (int i = 0; i < (num - 1); ++i) {
                while (*arg++)
                        ;
                data[i] = oarg;
                oarg = arg;
        }
        data[num - 1] = NULL;

        return data;
}


static inline void
print_data(const struct linked_list *const ll, const char *const vim_buf)
{
        for (struct Node *node = ll->head; node != NULL; node = node->next)
                if (strstr(vim_buf, CSS(node->data)->s) != NULL)
                        printf("%c\n%s\n", CSS(node->data)->kind, CSS(node->data)->s);
}


/* ========================================================================== */


static bool
in_order(const char *const *equiv, const char *order, char *group)
{
        /* `group' is actually a pointer to a char, not a C string. */
        for (; *equiv != NULL; ++equiv) {
                if (*group == (*equiv)[0]) {
                        /* eprintf("Group %c is the same as %c\n", *group, (*equiv)[1]); */
                        *group = (*equiv)[1];
                        break;
                }
        }

        return strchr(order, *group) != NULL;
}


static bool
is_correct_lang(const char *lang, const char *match_lang)
{
        if (strCeq(match_lang, lang))
                return true;

        if ((strCeq(lang, "C") || strCeq(lang, "C\\+\\+")) &&
            (strCeq(match_lang, "C++") || strCeq(match_lang, "C")))
                return true;

        return false;
}


static bool
skip_tag(const char *const *skip, const char *find)
{
        const char *buf;

        while ((buf = *skip++) != NULL)
                if (streq(buf, find))
                        return true;

        return false;
}


static struct linked_list *
search(const struct linked_list *taglist,
       const char *lang,
       const char *order,
       const char *const *skip,
       const char *const *equiv)
{
#define cur_str CSS(node->data)->s
        struct linked_list *ll = new_list(false);
        struct Node *node = taglist->tail;
        int nfields = 0;
        char *tok, *name, *match_lang;
        char kind;

        /* Skip past the comments and make sure the file isn't empty. */
        while (node != NULL && cur_str[0] == '!')
                node = node->prev;
        if (node == NULL) {
                warnx("Empty file!");
                goto error;
        }

        /* Verify that the file has the 2 required 'extra' fields. */
        char *tmp = STRDUP(cur_str);
        while ((tok = strsep(&tmp, "\t")) != NULL)
                if ((tok[0] != '\0' && tok[1] == '\0') ||
                    strncmp(tok, "language:", 9) == 0)
                        ++nfields;
#ifndef HAVE_STRDUPA
        free(tmp);
#endif
        if (nfields != 2) {
                warnx("Invalid file! nfields is %d", nfields);
                goto error;
        }

        for (; node != NULL; node = node->prev) {
                /* The name is first, followed by two fields we don't need. */
                name    = strsep(&cur_str, "\t");
                cur_str = strchr(cur_str, '\t');
                cur_str = strchr(cur_str, '\t');

                while ((tok = strsep(&cur_str, "\t")) != NULL) {
                        /* The 'kind' field is the only one that is 1 character
                         * long, and the 'language' field is prefaced. */
                        if (tok[0] != '\0' && tok[1] == '\0')
                                kind = *tok;
                        else if (strncmp(tok, "language:", 9) == 0)
                                match_lang = tok + 9;
                }

                /* Prune tags. Include only those that are:
                 *    1) of a type in the `order' list,
                 *    2) of the correct language (applies mainly to C
                 *       and C++, generally ctags filters languages),
                 *    3) are not included in the `skip' list, and
                 *    4) are not duplicates.
                 * If invalid, just move on. */
                if ( in_order(equiv, order, &kind) &&
                     is_correct_lang(lang, match_lang) &&
                    !skip_tag(skip, name) &&
                    !ll_find_s_string(ll, kind, name))
                {
                        struct string *tmp = xmalloc(sizeof *tmp);
                        tmp->s    = name;
                        tmp->kind = kind;
                        /* tmp->len = 0; */
                        ll_add(ll, tmp); 
                } 
        }

        return ll;

error:
        destroy_list(ll);
        return NULL;
}
