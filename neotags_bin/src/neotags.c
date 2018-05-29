#include "neotags.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef USE_PTHREADS
#  include <pthread.h>
#endif

struct pdata {
        int threadnum;
        const struct lldata *vim_buf;
        const char *lang;
        const char *order;
        const char *const *skip;
        const char *const *equiv;
        struct lldata **lst;
        int num;
};

static void search(
        struct datalist *tags, const struct lldata *vim_buf,
        const char *lang, const char *order,
        const char *const *skip, const char *const *equiv
);

static int ll_cmp(const void *vA, const void *vB);
static void *do_search(void *vdata);
static char **get_colon_data(char *oarg);

#ifdef DOSISH
#  define __CONST__
#  define SEPCHAR ';'
#else
#  define __CONST__ const
#  define SEPCHAR ':'
#endif

#define REQUIRED_INPUT 8
#define CCC(ARG_) ((const char *const *)(ARG_))

#define START() gettimeofday(&tv1, NULL)
#define END(STR_)                                                         \
        do {                                                              \
                gettimeofday(&tv2, NULL);                                 \
                eprintf("%s: Total time = %f seconds\n", (STR_),          \
                        ((double)(tv2.tv_usec - tv1.tv_usec) / 1000000) + \
                        (double)(tv2.tv_sec - tv1.tv_sec));               \
        } while (0)

static int files_read;


int
main(int argc, char *argv[])
{
        struct timeval tv1, tv2, tv3;
        gettimeofday(&tv3, NULL);
        setlocale(LC_NUMERIC, "");
        files_read   = 0;
        program_name = handle_progname(*argv++);
        if (isatty(0))
                errx(1, "This program can't be run manually.");
        if (--argc != REQUIRED_INPUT)
                errx(2, "Error: Wrong number of paramaters (%d, need %d).",
                     argc, REQUIRED_INPUT);
        eputs("Program ID: " PROG_ID "\n");

        char **files   = get_colon_data(*argv++);
        char *ctlang   = *argv++;
        char *vimlang  = *argv++;
        char *order    = *argv++;
        bool strip_com = xatoi(*argv++);
        int64_t nchars = xatoi(*argv++);
        char **skip    = get_colon_data(*argv++);
        char **equiv   = get_colon_data(*argv++);

        START();
        struct datalist tags = {
                .data = xmalloc(sizeof(*tags.data) * INIT_TAGS),
                .num  = 0,
                .max  = INIT_TAGS
        };

        for (char **ptr = files; *ptr != NULL; ptr += 2)
                files_read += getlines(&tags, *ptr, *(ptr + 1));
        if (files_read == 0)
                errx(1, "Error: no files were successfully read.");

        struct lldata vim_buf = {
            .s    = xmalloc(nchars + 1),
            .len  = nchars + 1
        };

        fread(vim_buf.s, 1, nchars, stdin);
        vim_buf.s[nchars] = '\0';

        if (strip_com) {
                warnx("Stripping comments...\n");
                strip_comments(&vim_buf, vimlang);
        }
        END("Finished reading files & stripping comments");

        search(&tags, &vim_buf, ctlang, order, CCC(skip), CCC(equiv));

        /* pointlessly free everything */
        for (int i = 0; i < backup_iterator; ++i)
                free(backup_pointers[i]);
        for (int i = 0; i < tags.num; ++i)
                free(tags.data[i]);

        free_all(equiv, files, skip, tags.data, vim_buf.s);

        tv1 = tv3;
        END("Done");
        return 0;
}

/* ========================================================================== */

static char **
get_colon_data(char *oarg)
{
        int num = 0;
        char *arg = oarg;

        if (*arg != '\0') {
                do if (*arg == SEPCHAR) {
                        *arg++ = '\0';
                        ++num;
                } while (*arg++);
        }

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


static int  /* Comparison function for qsort */
ll_cmp(const void *vA, const void *vB)
{
        int ret;
        const struct lldata *A = (*(struct lldata *const*)vA);
        const struct lldata *B = (*(struct lldata *const*)vB);

        if (A->kind == B->kind) {
                if (A->len == B->len)
                        ret = memcmp(A->s, B->s, A->len);
                else
                        ret = A->len - B->len;
        } else
                ret = A->kind - B->kind;

        return ret;
}


/* ========================================================================== */


static bool
in_order(const char *const *equiv, const char *order, char *group)
{
        /* `group' is actually a pointer to a char, not a C string. */
        for (; *equiv != NULL; ++equiv) {
                if (*group == (*equiv)[0]) {
                        *group = (*equiv)[1];
                        break;
                }
        }

        return strchr(order, *group) != NULL;
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


/*============================================================================*/


static void
search(struct datalist *tags,
       const struct lldata *vim_buf,
       const char *lang,
       const char *order,
       const char *const *skip,
       const char *const *equiv)
{
        if (tags->num == 0) {
                warnx("No tags found!");
                return;
        }
        struct timeval tv1, tv2;

#ifdef USE_PTHREADS
        int num_threads = find_num_cpus();
        pthread_t tid[num_threads];

        if (num_threads == 0)
                num_threads = 4;
        warnx("Using %d cpus.", num_threads);
        START();

        for (int i = 0; i < num_threads; ++i) {
                struct pdata *tmp = xmalloc(sizeof *tmp);
                int quot = (int)tags->num / num_threads;
                int num  = (i == num_threads - 1)
                              ? (int)(tags->num - ((num_threads - 1) * quot))
                              : quot;

                *tmp = (struct pdata){i, vim_buf, lang, order, skip, equiv,
                                      tags->data + (i * quot), num};

                errno = 0;
                if (pthread_create(tid + i, 0, do_search, tmp) != 0)
                        err(1, "pthread_create failed");
        }

        /* struct datalist **out = xmalloc(num_threads * sizeof(*out)); */
        struct datalist *out[num_threads];
        for (int th = 0; th < num_threads ; ++th) {
                void *tmp;
                pthread_join(tid[th], &tmp);
                out[th] = tmp;
        }

        END("All threads returned");

        uint32_t total = 0, offset = 0;
        for (int T = 0; T < num_threads; ++T)
                total += out[T]->num;

        struct lldata **alldata = xmalloc(total * sizeof(*alldata));

        for (int T = 0; T < num_threads; ++T) {
                if (out[T]->num > 0) {
                        memcpy(alldata + offset, out[T]->data,
                               out[T]->num * sizeof(*out));
                        offset += out[T]->num;
                }
                free(out[T]->data);
                free(out[T]);
        }
        /* free(out); */

#else /* USE_PTHREADS */

        warnx("Using 1 cpu (no threading available).");

        struct pdata *Pdata = xmalloc(sizeof *Pdata);
        *Pdata = (struct pdata){ 0, vim_buf, lang, order, skip, equiv,
                                     tags->data, tags->num };

        void *tmp = do_search(Pdata);

        struct lldata **alldata = ((struct datalist *)(tmp))->data;
        uint32_t total = ((struct datalist *)(tmp))->num;
#endif
        if (total == 0)
                goto cleanup;

        START();
        qsort(alldata, total, sizeof(*alldata), &ll_cmp);
        END("Finished sorting with qsort");

        /* Always display the first item. */
        printf("%c\n%s\n", alldata[0]->kind, alldata[0]->s);

        for (uint32_t i = 1; i < total; ++i)
                if (alldata[i]->len != alldata[i - 1]->len
                    || memcmp(alldata[i]->s,
                              alldata[i - 1]->s,
                              alldata[i]->len) != 0)
                        printf("%c\n%s\n", alldata[i]->kind, alldata[i]->s);

        END("Finished displaying stuff.");

cleanup:
        for (uint32_t i = 0; i < total; ++i)
                free(alldata[i]);
        free(alldata);
}


static void *
do_search(void *vdata)
{
#  define cur_str (data->lst[i]->s)
        struct pdata *data    = vdata;
        struct datalist *ret  = xmalloc(sizeof *ret);
        struct lldata **rdata = xmalloc(data->num * sizeof(*rdata));

        *ret = (struct datalist){ rdata, 0, data->num };

        for (int i = 0; i < data->num; ++i) {
                if (cur_str[0] == '!')
                        continue;

                /* The name is first, followed by two fields we don't need. */
                char *name = strsep(&cur_str, "\t");
                size_t namelen = (cur_str - name - 1LLU);
                cur_str = strchr(cur_str, '\t');
                cur_str = strchr(cur_str, '\t');

                char *tok;
                char *match_lang = NULL;
                char kind = '\0';

                while ((tok = strsep(&cur_str, "\t")) != NULL) {
                        /* The 'kind' field is the only one that is 1 character
                         * long, and the 'language' field is prefaced. */
                        if (tok[0] != '\0' && tok[1] == '\0')
                                kind = *tok;
                        else if (strncmp(tok, "language:", 9) == 0)
                                match_lang = tok + 9;
                }

                if (!match_lang || !kind)
                        continue;

                /* Prune tags. Include only those that are:
                 *    1) of a type in the `order' list,
                 *    2) of the correct language (applies mainly to C
                 *       and C++, generally ctags filters languages),
                 *    3) are not included in the `skip' list, and
                 *    4) are present in the current vim buffer.
                 * If invalid, just move on. */
                if ( in_order(data->equiv, data->order, &kind) &&
                     is_correct_lang(data->lang, match_lang) &&
                    !skip_tag(data->skip, name) &&
                     strstr(data->vim_buf->s, name) != NULL)
                {
                        struct lldata *tmp = xmalloc(sizeof *tmp);
                        tmp->s    = name;
                        tmp->kind = kind;
                        tmp->len  = namelen;
                        ret->data[ret->num++] = tmp;
                }
        }

        free(vdata);
#ifdef USE_PTHREADS
        pthread_exit(ret);
#else
        return ret;
#endif
}
