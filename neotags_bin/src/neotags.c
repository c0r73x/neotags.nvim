#define PCRE2_CODE_UNIT_WIDTH 8

#include "neotags.h"
#include <pcre2.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct linked_list * search(
        const struct strlst *taglist, const char *lang, const char *order,
        const char *const *ctov, const char *const *skip
);
static void get_colon_delim_data(char **data, char *arg);
static void print_data(const struct linked_list *ll, const char *vim_buf);
static bool skip_tag(const char *const *skip, const char *find);
static bool is_correct_lang(const char *const *ctov, const char *lang,
                            const char *match_lang);
static void normalize_lang(char *buf, const char *lang, const size_t max);

#define REQUIRED_INPUT 8
#define PATSIZ 256
#define PATTERN_PT1 \
        "^([^\\t]+)\\t(?:[^\\t]+)\\t\\/(?:.+)\\/;\"\\t(\\w)\\tlanguage:("
#define PATTERN_PT2 "(?:\\[a-zA-Z]+)?)"

#define CCC(ARG) ((const char *const *)(ARG))
#define _substr(INDEX, SUBJECT, OVECTOR) \
        ((char *)((SUBJECT) + (OVECTOR)[(INDEX)*2]))
#define _substrlen(INDEX, OVECTOR) \
        ((int)((OVECTOR)[(2 * (INDEX)) + 1] - (OVECTOR)[2 * (INDEX)]))

enum { tNAME = 1, tKIND, tLANG };


int
main(int argc, char **argv)
{
        if (isatty(0))
                xerr(1, "This program can't be run manually.\n");
        if (argc < REQUIRED_INPUT)
                xerr(2, "Error: Insufficient input paramaters.\n");

        program_name  = *argv++;
        char *tagfile = *argv++;
        char *lang    = *argv++;
        char *order   = *argv++;
        long nchars   = xatoi(*argv++);
        long nskip    = xatoi(*argv++);
        long nctov    = xatoi(*argv++);
        char *vim_buf = xmalloc(nchars + 2);
        char **skip   = xmalloc(sizeof *skip * (nskip + 1));
        char **ctov   = xmalloc(sizeof *ctov * (nctov + 1));

        get_colon_delim_data(skip, *argv++);
        get_colon_delim_data(ctov, *argv++);
        struct strlst *taglist = get_all_lines(tagfile);
        long i;

        /* Slurp the whole vim_buf from the python code */
        for (i = 0; i < nchars;)
                vim_buf[i++] = (char)getchar();
        vim_buf[i] = '\0';

        struct linked_list *ll = search(taglist, lang, order, CCC(ctov), CCC(skip));
        print_data(ll, vim_buf);

        /* pointlessly free everything */
        destroy_list(ll);
        destroy_strlst(taglist);
        char *buf, **tmp = skip;
        while ((buf = *tmp++) != NULL)
                free(buf);
        tmp = ctov;
        while ((buf = *tmp++) != NULL)
                free(buf);
        free(skip);
        free(ctov);
        free(vim_buf);

        return 0;
}


static struct linked_list *
search(const struct strlst *taglist,
       const char *lang,
       const char *order,
       const char *const *ctov,
       const char *const *skip)
{
        struct linked_list *ll = new_list();

        char pat[PATSIZ], match_lang[PATSIZ];
        pcre2_match_data *match_data;
        PCRE2_SIZE erroroffset;
        int errornumber;
        char norm_lang[PATSIZ / 2];
        normalize_lang(norm_lang, lang, PATSIZ);

        snprintf(pat, PATSIZ, "%s%s%s", PATTERN_PT1, norm_lang, PATTERN_PT2);
        PCRE2_SPTR pattern = (PCRE2_SPTR)pat;

        pcre2_code *cre =
            pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS,
                          &errornumber, &erroroffset, NULL);

        if (cre == NULL) {
                PCRE2_UCHAR vim_buf[BUFSIZ];
                pcre2_get_error_message(errornumber, vim_buf, BUFSIZ);
                xerr(1, "PCRE2 compilation failed at offset %d: %s\n",
                     (int)erroroffset, vim_buf);
        }

        for (uint32_t iter = 0; iter < taglist->num; ++iter) {
                if (taglist->s[iter][0] == '!' || taglist->s[iter][0] == '\0')
                        continue;

                PCRE2_SPTR subject = (PCRE2_SPTR)taglist->s[iter];
                size_t subject_len = (size_t)(taglist->slen[iter] - 1);

                match_data = pcre2_match_data_create_from_pattern(cre, NULL);
                int rcnt   = pcre2_match(cre, subject, subject_len, 0,
                                         PCRE2_CASELESS, match_data, NULL);

                if (rcnt < 0) /* no match */
                        goto next;

                PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

#define substr(INDEX) _substr(INDEX, subject, ovector)
#define substrlen(INDEX) _substrlen(INDEX, ovector)

                int len    = substrlen(tNAME) + 2;
                char *data = xmalloc(len);
                data[0]    = substr(tKIND)[0];

                strlcpy(match_lang, substr(tLANG), substrlen(tLANG) + 1);
                strlcpy(data + 1, substr(tNAME), len - 1);

                /* Prune tags. Include only those that are:
                 *         1) of a type in the `order' list,
                 *         2) of the correct language (applies mainly to C and
                 *            C++, generally ctags filters for that),
                 *         3) are not included in the `skip' list, and
                 *         4) are not duplicates.
                 *    If invalid, just free and move on.
                 */
                if ( strchr(order, (int)data[0]) &&
                     is_correct_lang(ctov, lang, match_lang) &&
                    !skip_tag(skip, data + 1) &&
                    !ll_find_str(ll, data)
                   )
                        ll_add(ll, data);
                else
                        free(data);

        next:
                pcre2_match_data_free(match_data);
        }

        pcre2_code_free(cre);
        return ll;
}


static void
get_colon_delim_data(char **data, char *arg)
{
        int ch, it = 0, dit = 0;
        char buf[BUFSIZ];

        while ((ch = *arg++) != '\0') {
                if (ch == ':') {
                        buf[it] = '\0';
                        if ((data[dit++] = strdup(buf)) == NULL)
                                xerr(1, "strdup failed!\n");
                        it = 0;
                } else {
                        buf[it++] = (char)ch;
                }
        }

        data[dit] = NULL;
}


static void
print_data(const struct linked_list *ll, const char *vim_buf)
{
        struct Node *current = ll->head;

        /* Check whether the tag is present in the current nvim vim_buf */
        while (current != NULL) {
                if (strstr(vim_buf, current->data + 1) != NULL)
                        printf("%c\n%s\n", current->data[0], current->data + 1);

                current = current->next;
        }
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


static bool
is_correct_lang(const char *const *ctov,
                const char *lang,
                const char *match_lang)
{
        if (strCeq(match_lang, lang))
                return true;

        if ((strCeq(lang, "C") || strCeq(lang, "C\\+\\+")) &&
            (strCeq(match_lang, "C++") || strCeq(match_lang, "C")))
                return true;

        while (*ctov != NULL)
                if (strCeq(match_lang, *ctov++) && strCeq(lang, *ctov++))
                        return true;

        return false;
}


/* C and C++ should be considered equivalent as far as tags are concerned. */
static void
normalize_lang(char *buf, const char *const lang, const size_t max)
{
        if (strCeq(lang, "C") || strCeq(lang, "C\\+\\+"))
                strlcpy(buf, "(?:C(?:\\+\\+)?)", max);
        else
                strlcpy(buf, lang, max);
}
