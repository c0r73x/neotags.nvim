#define PCRE2_CODE_UNIT_WIDTH 8

#include "neotags.h"
#include <bsd/bsd.h>
#include <pcre2.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct linked_list * search(struct strlst *taglist,
                                   const char *lang, const char *order,
                                   char **ctov, char **skip);

static void sanity_check         (int argc, char **argv);
static void get_colon_delim_data (char ** data, char *arg);
static void print_data           (struct linked_list *ll, char *buffer);
static bool skip_tag             (char **skip, const char *find);
static bool is_correct_lang      (char **ctov, const char *lang, const char *match_lang);

#define REQUIRED_INPUT 8
#define PATSIZ 128
#define PATTERN_PT1 "^([^\\t]+)\\t(?:[^\\t]+)\\t\\/(?:.+)\\/;\"\\t(\\w)\\tlanguage:("
#define PATTERN_PT2 "(?:\\w+)?)"

#define substr(IND)    ((char *)(subject + ovector[(IND)*2]))
#define substrlen(IND) ((int)(ovector[(2*(IND))+1] - ovector[2*(IND)]))

enum mgid_e {
        tNAME = 1,
        tKIND,
        tLANG
};


int
main(int argc, char **argv)
{
        sanity_check(argc, argv++);

        char *tagfile = *argv++;
        char *lang    = *argv++;;
        char *order   = *argv++;
        long nchars   = xatoi(*argv++);
        long nskip    = xatoi(*argv++);
        long nctov    = xatoi(*argv++);
        char *buffer  = xmalloc(nchars + 2);
        char **skip   = xmalloc(sizeof *skip * (nskip + 1));
        char **ctov   = xmalloc(sizeof *ctov * (nctov + 1));

        get_colon_delim_data(skip, *argv++);
        get_colon_delim_data(ctov, *argv++);
        struct strlst *taglist = get_all_lines(tagfile);
        long i;

        /* Slurp the whole buffer from the python code */
        for (i = 0; i < nchars;)
                buffer[i++] = (char)getchar();
        buffer[i] = '\0';

        struct linked_list *ll = search(taglist, lang, order, ctov, skip);
        print_data(ll, buffer);

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
        free(buffer);

        return 0;
}


static void
sanity_check(int argc, char **argv)
{
        program_name = *argv;
        if (isatty(0))
                xerr(1, "This program can't be run manually.\n");
        if (argc < REQUIRED_INPUT)
                xerr(2, "Error: Insufficient input paramaters.\n");
}


static void
get_colon_delim_data(char **data, char *arg)
{
        int ch, it = 0, dit = 0;
        char buf[BUFSIZ];
        eprintf("%s\n", arg);

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


static struct linked_list *
search(struct strlst *taglist,
       const char *lang,
       const char *order,
       char **ctov,
       char **skip)
{
        struct linked_list *ll = new_list();

        char pat[PATSIZ], match_lang[PATSIZ];
        pcre2_match_data *match_data;
        PCRE2_SIZE erroroffset;
        int errornumber;

        snprintf(pat, PATSIZ, "%s%s%s", PATTERN_PT1, lang, PATTERN_PT2);
        PCRE2_SPTR pattern = (PCRE2_SPTR)pat;

        pcre2_code *cre = pcre2_compile(
                        pattern, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS,
                        &errornumber, &erroroffset, NULL
        );

        if (cre == NULL) {
                PCRE2_UCHAR buffer[BUFSIZ];
                pcre2_get_error_message(errornumber, buffer, BUFSIZ);
                xerr(1, "PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
        }

        for (uint32_t iter = 0; iter < taglist->num; ++iter)
        {
                if (taglist->s[iter][0] == '!' || taglist->s[iter][0] == '\0')
                        continue;

                PCRE2_SPTR subject = (PCRE2_SPTR)taglist->s[iter];
                size_t subject_len = (size_t)(taglist->slen[iter] - 1);

                match_data = pcre2_match_data_create_from_pattern(cre, NULL);
                int rcnt   = pcre2_match(cre, subject, subject_len, 0,
                                         PCRE2_CASELESS, match_data, NULL);

                if (rcnt < 0)  /* no match */
                        goto next;

                PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

                int len    = substrlen(tNAME) + 2;
                char *data = xmalloc(len);
                data[0]    = substr(tKIND)[0];

                strlcpy(match_lang, substr(tLANG), substrlen(tLANG) + 1);
                strlcpy(data + 1, substr(tNAME), len - 1);

                if (    strchr(order, (int)data[0])  /* Is tag in order list? */
                    &&  is_correct_lang(ctov, lang, match_lang) /* Correct language? */
                    && !skip_tag(skip, data + 1)     /* Is tag in 'skip' list? */
                    && !ll_find_str(ll, data)        /* Is tag a duplicate? */
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
print_data(struct linked_list *ll, char *buffer)
{
        struct Node *current = ll->head;

        /* Check whether the tag is present in the current nvim buffer */
        while (current != NULL) {
                if (strstr(buffer, (current->data + 1)) != NULL)
                        printf("%c\n%s\n", current->data[0], current->data + 1);

                current = current->next;
        }
}


static bool
skip_tag(char **skip, const char *find)
{
        char *buf;
        char * * tmp = skip;

        while ((buf = *tmp++) != NULL)
                if (streq(buf, find)) 
                        return true;

        return false;
}


static bool
is_correct_lang(char **ctov, const char *lang, const char *match_lang)
{
        if (strCeq(match_lang, lang))
                return true;
        
        char * * tmp = ctov;
        while (*tmp != NULL)
                if (strCeq(match_lang, *tmp++) && strCeq(lang, *tmp++))
                        return true;

        if ((strCeq(lang, "C") || strCeq(lang, "C++")) &&
            (strCeq(match_lang, "C") || strCeq(match_lang, "C++")))
                return true;

        return false;
}
