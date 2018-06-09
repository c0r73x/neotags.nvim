#include "neotags.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef USE_PTHREADS
#  error Pthread support is required.
#endif

#ifdef DOSISH
#  include <malloc.h>
#  define __CONST__
#  define SEPCHAR ';'
#else
#  include <alloca.h>
#  define __CONST__ const
#  define SEPCHAR ':'
#endif

#define REQUIRED_INPUT 8
#define CCC(ARG_) ((const char *const *)(ARG_))
#define FREE_LIST(LST_, NUM_)                    \
        do {                                     \
                for (int i = 0; i < (NUM_); ++i) \
                        free((LST_)[i]);         \
        } while (0)

static char **get_colon_data(char *oarg);
static void print_tags(const struct StringLst *lst);
static struct StringLst *tok_search(struct StringLst *tags,  struct StringLst *vimbuf,
                                    const char *lang,        const char *order,
                                    const char *const *skip, const char *const *equiv);
static void *do_tok_search(void *vdata);


int
main(int argc, char *argv[])
{
        int files_read = 0;
        program_name   = handle_progname(*argv++);
        if (isatty(0))
                errx(1, "This program can't be run manually.");
        if (--argc != REQUIRED_INPUT)
                errx(2, "Error: Wrong number of paramaters (%d, need %d).",
                     argc, REQUIRED_INPUT);
        eputs("Program ID: " PROG_ID "\n");

        /* The only thing that uses this program is neotags.py, so the input is
         * guarenteed to be provided in the following order. */
        char ** files     = get_colon_data(*argv++);
        char  * ctlang    = *argv++;
        char  * vimlang   = *argv++;
        char  * order     = *argv++;
        bool    strip_com = xatoi(*argv++);
        size_t  nchars    = xatoi(*argv++);
        char ** skip      = get_colon_data(*argv++);
        char ** equiv     = get_colon_data(*argv++);

        struct StringLst tags = {
                .data = nmalloc(INIT_TAGS, sizeof(*tags.data)),
                .num  = 0,
                .max  = INIT_TAGS
        };

        /* Read all of the tag files and combine the tags into one list. */
        for (char **ptr = files; *ptr != NULL; ptr += 2)
                files_read += getlines(&tags, *ptr, *(ptr + 1));
        if (files_read == 0)
                errx(1, "Error: no files were successfully read.");

        /* Get the contents of the vim buffer from the standard input. */
        struct String vim_buf = { .s = malloc(nchars + 1llu) };
        if ((vim_buf.len = fread(vim_buf.s, 1, nchars, stdin)) != nchars)
                warn("Read error => size: %ld, read: %zu", nchars, vim_buf.len);
        vim_buf.s[vim_buf.len] = '\0';

        if (strip_com) {
                warnx("Stripping comments...\n");
                strip_comments(&vim_buf, vimlang);
        }

        /* Crudely tokenize the vim buffer into words, discarding punctuation
         * and hopefully not any identifiers. The returned list is sorted and
         * ready for use with bsearch. */
        struct StringLst *toks = tokenize(&vim_buf);
        struct StringLst *lst  = tok_search(&tags, toks, ctlang, order,
                                            CCC(skip), CCC(equiv));
        print_tags(lst);

        /* Pointlessly free everything. */
        FREE_LIST(backup_pointers, backup_iterator);
        FREE_LIST(tags.data, tags.num);
        FREE_LIST(toks->data, toks->num);
        FREE_LIST(lst->data, lst->num);
        free_all(equiv, files, skip, tags.data, vim_buf.s, toks->data, toks,
                 lst->data, lst);

        return 0;
}

/* ========================================================================== */

static char **
get_colon_data(char *oarg)
{
        char sep[2], *tok, *arg = oarg;
        char **data, **odata;
        int num = 0;

        while (*arg && (arg = strchr(arg, SEPCHAR)))
                ++num, ++arg;

        data = odata = nmalloc(num + 2, sizeof(*data));
        arg = oarg;
        sep[0] = SEPCHAR;
        sep[1] = '\0';

        while ((tok = strsep(&arg, sep)))
                *data++ = tok;

        if (**(data - 1))
                *data = NULL;
        else
                *(--data) = NULL;

        return odata;
}


/* Compares two struct Strings in a reasonably efficient manner, avoiding actual
 * lexical comparisson unless it is absolutely necessary. The results won't be
 * "properly" sorted alphabetically, but that doesn't matter - the only
 * requirement here is that identical strings end up adjacent to one another. */
static int
tag_cmp(const void *vA, const void *vB)
{
        int ret;
        const struct String *A = *((struct String **)(vA));
        const struct String *B = *((struct String **)(vB));

        if (A->kind == B->kind) {
                if (A->len == B->len)
                        ret = memcmp(A->s, B->s, A->len);
                else
                        ret = A->len - B->len;
        } else
                ret = A->kind - B->kind;

        return ret;
}


/* Also compares two struct Strings, but ignores the 'kind' field. */
static int
s_string_cmp(const void *vA, const void *vB)
{
        const struct String *A = *((struct String **)(vA));
        const struct String *B = *((struct String **)(vB));
        int ret = 0;

        if (A->len == B->len)
                ret = memcmp(A->s, B->s, A->len);
        else
                ret = A->len - B->len;

        return ret;
}


static void
print_tags(const struct StringLst *lst)
{
#define DATA (lst->data)
        /* Always print the first tag. */
        printf("%c\n%s\n", DATA[0]->kind, DATA[0]->s);

        for (uint32_t i = 1; i < lst->num; ++i)
                if (DATA[i]->len != DATA[i-1]->len
                    || memcmp(DATA[i]->s,
                              DATA[i-1]->s,
                              DATA[i]->len) != 0)
                        printf("%c\n%s\n", DATA[i]->kind, DATA[i]->s);
}

#undef DATA

/* ========================================================================== */


static bool
in_order(const char *const *equiv, const char *order, char *kind)
{
        /* `kind' is actually a pointer to a char, not a C string. */
        for (; *equiv != NULL; ++equiv)
                if (*kind == (*equiv)[0]) {
                        *kind = (*equiv)[1];
                        break;
                }

        return strchr(order, *kind) != NULL;
}


static bool
is_correct_lang(const char *lang, __CONST__ char *match_lang)
{
#ifdef DOSISH
        /* It's a little disgusting to have to strlen every single string in
         * Windows just to get ride of some '\r's, but it must be done. */
        size_t size = strlen(match_lang);
        if (match_lang[size - 1] == '\r')
                match_lang[size - 1] = '\0';
#endif
        if (strCeq(match_lang, lang))
                return true;

        return ((strCeq(lang, "C") || strCeq(lang, "C\\+\\+")) &&
                (strCeq(match_lang, "C") || strCeq(match_lang, "C++")));
}


static bool
skip_tag(const char *const *skip, const char *find)
{
        const char *buf;

        while ((buf = *skip++))
                if (streq(buf, find))
                        return true;

        return false;
}


/*============================================================================*/


struct pdata {
        const struct StringLst *vim_buf;
        const char *lang;
        const char *order;
        const char *const *skip;
        const char *const *equiv;
        struct String **lst;
        int num;
        int threadnum;
};


static struct StringLst *
tok_search(struct StringLst *tags,
           struct StringLst *vimbuf,
           const char *lang,
           const char *order,
           const char *const *skip,
           const char *const *equiv)
{
#define vim_d  (vimbuf->data)
#define uniq_d (uniq->data)
        if (tags->num == 0)
                errx(1, "No tags found!");

        int num_threads = find_num_cpus();
        if (num_threads == 0)
                num_threads = 4;

        uint32_t total, offset;
        pthread_t         *tid = alloca(num_threads * sizeof(*tid));
        struct StringLst **out = alloca(num_threads * sizeof(*out));
        warnx("Sorting through %ld tags with %d cpus.", tags->num, num_threads);

        /* Because we may have examined multiple tags files, it's very possible
         * for there to be duplicate tags. Sort the list and remove any. */
        qsort(vim_d, vimbuf->num, sizeof(*vim_d), &s_string_cmp);

        struct StringLst *uniq = malloc(sizeof *uniq);
        *uniq = (struct StringLst){
                .data = nmalloc(vimbuf->num, sizeof(*uniq->data)),
                .num  = 1,
                .max  = vimbuf->num
        };
        uniq_d[0] = vim_d[0];

        for (int i = 1; i < vimbuf->num; ++i)
                if (vim_d[i]->len != vim_d[i-1]->len
                    || memcmp(vim_d[i]->s,
                              vim_d[i-1]->s,
                              vim_d[i]->len) != 0)
                        uniq_d[uniq->num++] = vim_d[i];

        /* Launch the actual search in separate threads, with each handling as
         * close to an equal number of tags as the math allows. */
        for (int i = 0; i < num_threads; ++i) {
                struct pdata *tmp = malloc(sizeof *tmp);
                int quot = (int)tags->num / num_threads;
                int num  = (i == num_threads - 1)
                              ? (int)(tags->num - ((num_threads - 1) * quot))
                              : quot;

                *tmp = (struct pdata){uniq, lang, order, skip, equiv,
                                          tags->data + (i * quot), num, i};

                if (pthread_create(tid + i, NULL, &do_tok_search, tmp) != 0)
                        err(1, "pthread_create failed");
        }

        /* Collect the threads. */
        for (int i = 0; i < num_threads; ++i)
                pthread_join(tid[i], (void **)(&out[i]));

        free_all(uniq->data, uniq);
        total = offset = 0;

        for (int T = 0; T < num_threads; ++T)
                total += out[T]->num;
        if (total == 0)
                errx(0, "No tags found in buffer.");

        /* Combine the returned data from all threads into one array, which is
         * then sorted and returned. */
        struct String **alldata = nmalloc(total, sizeof(*alldata));
        struct StringLst *ret   = malloc(sizeof *ret);
        *ret = (struct StringLst){ alldata, total, total };

        for (int T = 0; T < num_threads; ++T) {
                if (out[T]->num > 0) {
                        memcpy(alldata + offset, out[T]->data,
                               out[T]->num * sizeof(*out));
                        offset += out[T]->num;
                }
                free_all(out[T]->data, out[T]);
        }

        qsort(alldata, total, sizeof(*alldata), &tag_cmp);

        return ret;
}


static void *
do_tok_search(void *vdata)
{
#define cur_str (data->lst[i]->s)
        struct pdata     *data  = vdata;
        struct String   **rdata = nmalloc(data->num, sizeof(*rdata));
        struct StringLst *ret   = malloc(sizeof *ret);
        *ret = (struct StringLst){ rdata, 0, data->num };

        for (int i = 0; i < data->num; ++i) {
                /* Skip empty lines and comments. */
                if (!cur_str[0] || cur_str[0] == '!')
                        continue;

                /* The name is first, followed by two fields we don't need. */
                char *name = strsep(&cur_str, "\t");
                size_t namelen = (cur_str - name - 1LLU);
                cur_str = strchr(cur_str, '\t');
                cur_str = strchr(cur_str, '\t');

                char *tok, *match_lang = NULL;
                char kind = '\0';

                /* Extract the 'kind' and 'language' fields. The former is the
                 * only one that is 1 character long, and the latter is prefaced. */
                while ((tok = strsep(&cur_str, "\t"))) {
                        if (tok[0] != '\0' && tok[1] == '\0')
                                kind = *tok;
                        else if (strncmp(tok, "language:", 9) == 0)
                                match_lang = tok + 9;
                }

                if (!match_lang || !kind)
                        continue;

                struct String name_s_tmp = { .s = name, .len = namelen };
                struct String *name_s = &name_s_tmp;

                /* Prune tags. Include only those that are:
                 *    1) of a type in the `order' list,
                 *    2) of the correct language,
                 *    3) are not included in the `skip' list, and
                 *    4) are present in the current vim buffer.
                 * If invalid, just move on. */
                if ( in_order(data->equiv, data->order, &kind) &&
                     is_correct_lang(data->lang, match_lang) &&
                    !skip_tag(data->skip, name) &&
                     bsearch(&name_s, data->vim_buf->data, data->vim_buf->num,
                             sizeof(*data->vim_buf->data), &s_string_cmp))
                {
                        struct String *tmp = malloc(sizeof *tmp);
                        tmp->s    = name;
                        tmp->kind = kind;
                        tmp->len  = namelen;
                        ret->data[ret->num++] = tmp;
                }
        }

        free(vdata);

#ifndef DOSISH
        pthread_exit(ret);
#else
        return ret;
#endif
}
