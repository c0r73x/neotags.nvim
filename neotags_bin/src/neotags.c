#include "neotags.h"
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef DOSISH
#  define __CONST__
#  define STARTCOUNT()
#  define ENDCOUNT(STR_)
#else
#  include <sys/time.h>
#  define __CONST__ const
#  define STARTCOUNT() gettimeofday(&tv1, NULL)
#  define ENDCOUNT(STR_)                                                    \
        do {                                                              \
                gettimeofday(&tv2, NULL);                                 \
                eprintf("%s: Total time = %f seconds\n", (STR_),          \
                        ((double)(tv2.tv_usec - tv1.tv_usec) / 1000000) + \
                         (double)(tv2.tv_sec - tv1.tv_sec));              \
        } while (0)
struct timeval tv1, tv2;
#endif

struct pdata {
        int threadnum;
        const char *vim_buf;
        const char *lang;
        const char *order;
        const char *const *skip;
        const char *const *equiv;
        struct string **lst;
        int num;
};

struct strlist {
        struct string **data;
        int64_t num;
};

#ifdef USE_PTHREADS
#include <pthread.h>
static void search(
        struct linked_list *taglist, const char *lang, const char *order,
        const char *vim_buf, const char *const *skip, const char *const *equiv
);
static void *do_search(void *vdata);
#else
static struct linked_list *no_thread_search(
        const struct linked_list *taglist, const char *lang,
        const char *order, const char *const *skip, const char *const *equiv
);
static inline void print_data(const struct linked_list *ll, const char *vim_buf);
#endif

static char **get_colon_data(char *oarg);

#ifdef HAVE_STRDUPA
#  define STRDUP strdupa
#else
#  define STRDUP strdup
#endif
#define REQUIRED_INPUT 8
#define CCC(ARG_) ((const char *const *)(ARG_))
#define CSS(NODE_) ((struct string *)(NODE_))


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

        struct linked_list *taglist = new_list(ST_STRING_NOFREE);
        warnx("ctlang: %s, vimlang: %s\n", ctlang, vimlang);
        dump_list(files);
        dump_list(equiv);

        STARTCOUNT();
        for (char **ptr = files; *ptr != NULL; ptr += 2)
                reads += getlines(taglist, *ptr, *(ptr + 1));

        if (reads == 0)
                errx(1, "Error: no files were successfully read.");

        fread(vim_buf, 1, nchars, stdin);
        vim_buf[nchars] = '\0';
        ENDCOUNT("Raading tag files");

        STARTCOUNT();
        if (strip_com) {
                warnx("Stripping comments...\n");
                struct string tmp = {vim_buf, '\0', nchars + 1};
                char *buf = strip_comments(&tmp, vimlang);
                if (buf) {
                        free(vim_buf);
                        vim_buf = buf;
                }
        }
        ENDCOUNT("Stripping comments");

        STARTCOUNT();
#ifdef USE_PTHREADS
        search(taglist, ctlang, order, vim_buf, CCC(skip), CCC(equiv));
#else
        struct linked_list *ll =
            no_thread_search(taglist, ctlang, order, CCC(skip), CCC(equiv));
        if (ll) {
                print_data(ll, vim_buf);
                destroy_list(ll);
        }
#endif
        ENDCOUNT("Doing search");

        /* pointlessly free everything */
        for (int i = 0; i < backup_iterator; ++i)
                free(backup_pointers[i]);
        destroy_list(taglist);
        free_all(files, skip, equiv, vim_buf);

        return 0;
}


#ifdef DOSISH
#  define SEPCHAR ';'
#else
#  define SEPCHAR ':'
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
search(struct linked_list *taglist,
       const char *lang,
       const char *order,
       const char *vim_buf,
       const char *const *skip,
       const char *const *equiv)
{
        /* Skip past the comments and make sure the file isn't empty. */
        struct Node *node = taglist->tail;
        while (node != NULL && CSS(node->data)->s[0] == '!') {
                node = node->prev;
                ll_remove(taglist, -1);
        }
        if (node == NULL) {
                warnx("Empty file!");
                return;
        }
        
        struct strlist *tags = xmalloc(sizeof *tags);
        *tags = (struct strlist){
                .data = xmalloc(sizeof(struct string *) * taglist->size),
                .num  = taglist->size
        };

        node = taglist->tail;
        for (int i = 0; node != NULL; node = node->prev, ++i)
                tags->data[i] = node->data;

        int num_threads = find_num_cpus();
        if (num_threads == 0)
                num_threads = 4;
        warnx("Using %d cpus.", num_threads);
        pthread_t tid[num_threads];

        for (int i = 0; i < num_threads; ++i) {
                struct pdata *tmp = xmalloc(sizeof *tmp);
                int div = (taglist->size / (num_threads));

                int num = (i == num_threads - 1)
                              ? (int)(taglist->size - ((num_threads - 1) * div))
                              : div;

                *tmp = (struct pdata){
                        .threadnum = i,
                        .vim_buf   = vim_buf,
                        .lang  = lang,
                        .order = order,
                        .skip  = skip,
                        .equiv = equiv,
                        .lst   = tags->data + (i * div),
                        .num   = num
                };

                errno = 0;
                int pt = pthread_create(tid + i, 0, do_search, tmp);
                if (pt != 0 || errno)
                        err(1, "pthread_create failed");
        }

        struct strlist **out = xmalloc(sizeof(*out) * num_threads);

        for (int th = 0; th < num_threads ; ++th) {
                void *tmp;
                pthread_join(tid[th], &tmp);
                out[th] = tmp;
        }

#  define last out[T]->num - 1
#  define cross_arr_duplicate()                                    \
        (T < num_threads - 1 &&                                    \
         (out[T]->data[last]->kind == out[T + 1]->data[0]->kind && \
          streq(out[T]->data[last]->s, out[T + 1]->data[0]->s)))


        for (int T = 0; T < num_threads; ++T) {
                /* int end = cross_arr_duplicate() ? out[T]->num - 1
                                                : out[T]->num; */
                int end = out[T]->num;

                for (int i = 0; i < end; ++i) {
                        printf("%c\n%s\n",
                               out[T]->data[i]->kind,
                               out[T]->data[i]->s);
                        free(out[T]->data[i]);
                }

                free_all(out[T]->data, out[T]);
        }

        free_all(tags->data, tags, out);
}


static void *
do_search(void *vdata)
{
        struct pdata *data = vdata;
        char *tok, *name, *match_lang;
        char kind;

        struct strlist *ret = xmalloc(sizeof *ret);
        *ret = (struct strlist){
                .data = xmalloc(sizeof(struct string *) * data->num),
                .num  = 0
        };

#  define cur_str  (data->lst[i]->s)
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
                        struct string *tmp = xmalloc(sizeof *tmp);
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


static struct linked_list *
no_thread_search(const struct linked_list *taglist,
                 const char *lang,
                 const char *order,
                 const char *const *skip,
                 const char *const *equiv)
{
#  define cur_str CSS(node->data)->s
        struct linked_list *ll = new_list(ST_STRING_NOFREE);
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
#  ifndef HAVE_STRDUPA
        free(tmp);
#  endif
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
                        ll_add(ll, tmp); 
                } 
        }

        return ll;

error:
        destroy_list(ll);
        return NULL;
}


static inline void
print_data(const struct linked_list *const ll, const char *const vim_buf)
{
        for (struct Node *node = ll->head; node != NULL; node = node->next)
                if (strstr(vim_buf, CSS(node->data)->s) != NULL)
                        printf("%c\n%s\n", CSS(node->data)->kind, CSS(node->data)->s);
}

#endif /* USE_PTHREADS */
