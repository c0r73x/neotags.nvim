#include "neotags.h"
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_PTHREADS
#  include <pthread.h>
   static void *do_search(void *vdata);

   struct pdata {
           int threadnum;
           const char *vim_buf;
           const char *lang;
           const char *order;
           const char *const *skip;
           const char *const *equiv;
           struct lldata **lst;
           int num;
   };
#endif

static void search(
        struct datalist *taglist, const char *vim_buf, const char *lang,
        const char *order, const char *const *skip, const char *const *equiv
);
static char **get_colon_data(char *oarg);

#ifdef DOSISH
#  define __CONST__
#  define SEPCHAR ';'
#else
#  define __CONST__ const
#  define SEPCHAR ':'
#endif

#ifdef HAVE_STRDUPA
#  define STRDUP strdupa
#else
#  define STRDUP strdup
#endif

#define REQUIRED_INPUT 8
#define CCC(ARG_)  ((const char *const *)(ARG_))
#define DATA(NODE_) ((struct lldata *)(NODE_))


int
main(int argc, char *argv[])
{
        setlocale(LC_NUMERIC, "");
        int reads    = 0;
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

        /* struct linked_list *taglist = new_list(ST_STRING_NOFREE); */
        struct datalist *taglist = xmalloc(sizeof(*taglist));
        taglist->data = xmalloc(sizeof(*taglist->data) * INIT_TAGS);
        taglist->num = 0;
        taglist->max = INIT_TAGS;

        warnx("ctlang: %s, vimlang: %s\n", ctlang, vimlang);
        dump_list(files);
        dump_list(equiv);

        for (char **ptr = files; *ptr != NULL; ptr += 2)
                reads += getlines(taglist, *ptr, *(ptr + 1));

        if (reads == 0)
                errx(1, "Error: no files were successfully read.");

        fread(vim_buf, 1, nchars, stdin);
        vim_buf[nchars] = '\0';

        if (strip_com) {
                warnx("Stripping comments...\n");
                struct lldata tmp = {vim_buf, '\0', nchars + 1};
                char *buf = strip_comments(&tmp, vimlang);
                if (buf) {
                        free(vim_buf);
                        vim_buf = buf;
                }
        }

        struct lldata **backup_data = taglist->data;

        search(taglist, vim_buf, ctlang, order, CCC(skip), CCC(equiv));

        /* pointlessly free everything */
        for (int i = 0; i < backup_iterator; ++i)
                free(backup_pointers[i]);
        for (int i = 0; i < taglist->num; ++i)
                free(taglist->data[i]);
        free_all(backup_data, equiv, files, skip, taglist, vim_buf);

        return 0;
}


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
#ifdef USE_PTHREADS

static void
search(struct datalist *taglist,
       const char *vim_buf,
       const char *lang,
       const char *order,
       const char *const *skip,
       const char *const *equiv)
{
        /* Skip past the comments and make sure the file isn't empty. */
        int ia;
        for (ia = 0; ia < taglist->num && taglist->data[ia]->s[0] == '!'; ++ia)
                free(taglist->data[ia]);

        taglist->data += ia;
        taglist->num  -= ia;

        if (taglist->num == 0) {
                warnx("No tags found!");
                return;
        }
        
        int num_threads = find_num_cpus();
        if (num_threads == 0)
                num_threads = 4;
        warnx("Using %d cpus.", num_threads);

        pthread_t tid[num_threads];

        for (int i = 0; i < num_threads; ++i) {
                struct pdata *tmp = xmalloc(sizeof *tmp);
                int div = (taglist->num / num_threads);

                int num = (i == num_threads - 1)
                              ? (int)(taglist->num - ((num_threads - 1) * div))
                              : div;

                *tmp = (struct pdata){
                        .threadnum = i,
                        .vim_buf   = vim_buf,
                        .lang  = lang,
                        .order = order,
                        .skip  = skip,
                        .equiv = equiv,
                        .lst   = taglist->data + (i * div),
                        .num   = num
                };

                errno = 0;
                int pt = pthread_create(tid + i, 0, do_search, tmp);
                if (pt != 0 || errno)
                        err(1, "pthread_create failed");
        }

        struct datalist **out = xmalloc(sizeof(*out) * num_threads);

        for (int th = 0; th < num_threads ; ++th) {
                void *tmp;
                pthread_join(tid[th], &tmp);
                out[th] = tmp;
        }

        for (int T = 0; T < num_threads; ++T) {
                for (int i = 0; i < out[T]->num; ++i) {
                        printf("%c\n%s\n",
                               out[T]->data[i]->kind,
                               out[T]->data[i]->s);
                        free(out[T]->data[i]);
                }

                free(out[T]->data);
                free(out[T]);
        }

        free(out);
}


static void *
do_search(void *vdata)
{
        struct pdata *data = vdata;
        char *tok, *name, *match_lang;
        char kind;

        struct datalist *ret = xmalloc(sizeof *ret);
        *ret = (struct datalist){
                .data = xmalloc(sizeof(struct lldata *) * data->num),
                .num  = 0
        };

#  define cur_str (data->lst[i]->s)
#  define is_dup(KIND, NAME, PREV) \
        ((KIND) == (PREV)->kind && streq((NAME), (PREV)->s))


        for (int i = 0; i < data->num; ++i) {
                /* The name is first, followed by two fields we don't need. */
                name    = strsep(&cur_str, "\t");
                cur_str = strchr(cur_str, '\t');
                cur_str = strchr(cur_str, '\t');

                match_lang = NULL;
                kind = '\0';

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
                 *    4) are not duplicates, and
                 *    5) are present in the current vim buffer
                 * If invalid, just move on. */
                if ( in_order(data->equiv, data->order, &kind) &&
                     is_correct_lang(data->lang, match_lang) &&
                    !skip_tag(data->skip, name) &&
                     (ret->num == 0 || !is_dup(kind, name,
                                               ret->data[ret->num - 1])) &&
                     strstr(data->vim_buf, name) != NULL)
                {
                        struct lldata *tmp = xmalloc(sizeof *tmp);
                        tmp->s    = name;
                        tmp->kind = kind;
                        ret->data[ret->num++] = tmp;
                } else {
                        free(data->lst[i]->s);
                }
        }

        free(vdata);
        pthread_exit(ret);
}


/*============================================================================*/
#else /* USE_PTHREADS */


static void
search(struct datalist *taglist,
       const char *vim_buf,
       const char *lang,
       const char *order,
       const char *const *skip,
       const char *const *equiv)
{
#  define cur_str (taglist->data[i]->s)
        /* struct linked_list *ll = new_list(ST_STRING_NOFREE); */
        int nfields = 0;
        char *tok, *name, *match_lang;
        char kind;

        /* Skip past the comments and make sure the file isn't empty. */
        int ia;
        for (ia = 0; ia < taglist->num && taglist->data[ia]->s[0] == '!'; ++ia)
                free(taglist->data[ia]);

        taglist->data += ia;
        taglist->num  -= ia;

        if (taglist->num == 0) {
                warnx("No tags found!");
                return;
        }

        /* Verify that the file has the 2 required 'extra' fields. */
        char *tmp = STRDUP(taglist->data[0]->s);
        while ((tok = strsep(&tmp, "\t")) != NULL)
                if ((tok[0] != '\0' && tok[1] == '\0') ||
                    strncmp(tok, "language:", 9) == 0)
                        ++nfields;
#  ifndef HAVE_STRDUPA
        free(tmp);
#  endif
        if (nfields != 2) {
                warnx("Invalid file! nfields is %d", nfields);
                return;
        }

        struct lldata prev = { .s = NULL };

        /* for (; node != NULL; node = node->prev) { */
        for (int i = 0; i < taglist->num; ++i) {
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

#  define is_dup(KIND, NAME, PREV) \
        ((KIND) == (PREV).kind && streq((NAME), (PREV).s))

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
                     (!prev.s || !is_dup(kind, name, prev)) &&
                     strstr(vim_buf, name) != NULL)
                {
                        prev.s    = name;
                        prev.kind = kind;
                        printf("%c\n%s\n", kind, name);
                }
        }
}

#endif /* USE_PTHREADS */
