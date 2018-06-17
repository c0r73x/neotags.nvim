#include "neotags.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DOSISH
#  include <malloc.h>
#  define __CONST__
#  define SEPCHAR ';'
#else
#  include <alloca.h>
#  define __CONST__ const
#  define SEPCHAR ':'
#endif

#define REQUIRED_INPUT 9
#define free_list(LST)                               \
        do {                                         \
                for (int i = 0; i < (LST)->num; ++i) \
                        free((LST)->lst[i]);         \
        } while (0)

static const strlist *get_colon_data(char *oarg);
static void print_tags(const strlist *lst, const char *ft);
static strlist *tok_search(strlist *tags, strlist *vimbuf, const strlist *skip,
                           const strlist *equiv, const string *lang,
                           const string *order, const string *filename);
static void *do_tok_search(void *vdata);

bool is_c_or_cpp;


int
main(int argc, char *argv[])
{
        if (isatty(0))
                errx(1, "This program can't be run manually.");
        if (--argc != REQUIRED_INPUT)
                errx(2, "Error: Wrong number of paramaters (%d, need %d).",
                        argc, REQUIRED_INPUT);
        eputs("Program ID: " PROG_ID "\n");

        int nread = 0;
        program_name = handle_progname(*argv++);
        backup_pointers.lst = nmalloc((backup_pointers.max = 1024llu), sizeof(char *));

        /* The only thing that uses this program is neotags.py, so the input is
         * guarenteed to be provided in the following order. */
        const strlist *files    = get_colon_data(*argv++);
        const string  ctlang    = tostring(*argv++);
        const string  vimlang   = tostring(*argv++);
        const string  order     = tostring(*argv++);
        const bool    strip_com = xatoi(*argv++);
        const size_t  nchars    = xatoi(*argv++);
        const strlist *skip     = get_colon_data(*argv++);
        const strlist *equiv    = get_colon_data(*argv++);
        const string  filename  = tostring(*argv++);

        lang_id     = id_lang(&vimlang);
        is_c_or_cpp = (lang_id == _C_ || lang_id == _CPP_);

        strlist tags   = { nmalloc(INIT_TAGS, sizeof(*tags.lst)), 0, INIT_TAGS };
        string vim_buf = { malloc(nchars + 1llu), 0, 0 };

        /* Read all of the tag files and combine the tags into one list. */
        for (unsigned i = 0; i < files->num; i += 2)
                nread += getlines(&tags, files->lst[i]->s, files->lst[i+1]->s);
        if (nread == 0)
                errx(1, "Error: no files were successfully read.");

        /* Get the contents of the vim buffer from the standard input. */
        vim_buf.len            = fread(vim_buf.s, 1, nchars, stdin);
        vim_buf.s[vim_buf.len] = '\0';
        if (vim_buf.len != nchars)
                warn("Read error => size: %ld, read: %zu", nchars, vim_buf.len);

        if (strip_com)
                strip_comments(&vim_buf);

        /* Crudely tokenize the vim buffer into words, discarding punctuation
         * and hopefully not any identifiers. The returned list is sorted and
         * ready for use with bsearch. */
        strlist *toks = tokenize(&vim_buf);
        strlist *list = tok_search(&tags, toks, skip, equiv, &ctlang, &order, &filename);

        if (lang_id == _VIM_)
                print_tags_vim(list, vimlang.s);
        else
                print_tags(list, vimlang.s);

        /* Pointlessly free everything. */
        free_list(&backup_pointers);
        free_all_strlists(&tags, files, skip, equiv, toks, list);
        free_all(equiv->lst, (void *)equiv, files->lst, files, skip->lst, skip,
                 tags.lst, vim_buf.s, toks->lst, toks, list->lst, list, backup_pointers.lst);

        return 0;
}

/* ========================================================================== */

static const strlist *
get_colon_data(char *oarg)
{
        char sep[2], *tok, *arg = oarg;
        string **data, **odata;
        int num = 0;

        while (*arg && (arg = strchr(arg, SEPCHAR)))
                ++num, ++arg;

        data = odata = nmalloc(++num, sizeof(*data));
        arg = oarg;
        sep[0] = SEPCHAR;
        sep[1] = '\0';

        while ((tok = strsep(&arg, sep))) {
                *data    = malloc(sizeof **data);
                **data++ = tostring(tok);
        }

        strlist *ret = malloc(sizeof *ret);
        *ret = (strlist){ odata, num, num };
        return ret;
}


/* Compares two struct Strings in a reasonably efficient manner, avoiding actual
 * lexical comparision unless it is absolutely necessary. The results won't be
 * "properly" sorted alphabetically, but that doesn't matter - the only
 * requirement here is that identical strings end up adjacent to one another. */
static int
tag_cmp(const void *vA, const void *vB)
{
        int ret;
        const string *A = *(string **)(vA);
        const string *B = *(string **)(vB);

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
        int ret;
        const string *A = *(string **)(vA);
        const string *B = *(string **)(vB);

        if (A->len == B->len)
                ret = memcmp(A->s, B->s, A->len);
        else
                ret = A->len - B->len;

        return ret;
}


#define DATA (lst->lst)
#define PRINT(IT) (printf("%s#%c\t%s\n", ft, DATA[IT]->kind, DATA[IT]->s))

static void
print_tags(const strlist *lst, const char *ft)
{
        /* Always print the first tag. */
        PRINT(0);

        for (uint32_t i = 1; i < lst->num; ++i)
                if (!string_eq(DATA[i], DATA[i-1]))
                        PRINT(i);
}

#undef DATA
#undef PRINT


/* ========================================================================== */


static bool
in_order(const strlist *equiv, const string *order, char *kind)
{
        /* `kind' is actually a pointer to a char, not a C string. */
        for (unsigned i = 0; i < equiv->num; ++i)
                if (*kind == equiv->lst[i]->s[0]) {
                        *kind = equiv->lst[i]->s[1];
                        break;
                }

        return strchr(order->s, *kind) != NULL;
}


static bool
is_correct_lang(const string *lang, __CONST__ string *match_lang)
{
#ifdef DOSISH
        if (match_lang->s[match_lang->len - 1] == '\r')
                match_lang->s[match_lang->len - 1] = '\0';
#endif
        if (string_eq_i(match_lang, lang))
                return true;

        return (is_c_or_cpp && (string_lit_eq_i(match_lang, "C") ||
                                string_lit_eq_i(match_lang, "C++")));
}


static bool
skip_tag(const strlist *skip, const string *find)
{
        for (unsigned i = 0; i < skip->num; ++i)
                if (string_eq(skip->lst[i], find))
                        return true;

        return false;
}


/*============================================================================*/


#define vim_d  (vimbuf->lst)
#define uniq_d (uniq->lst)
struct pdata {
        const strlist *vim_buf;
        const strlist *skip;
        const strlist *equiv;
        const string *lang;
        const string *order;
        const string *filename;
        string **lst;
        int num;
};


static strlist *
tok_search(strlist *tags,
           strlist *vimbuf,
           const strlist *skip,
           const strlist *equiv,
           const string *lang,
           const string *order,
           const string *filename)
{
        if (tags->num == 0)
                errx(1, "No tags found!");

        int num_threads = find_num_cpus();
        if (num_threads <= 0)
                num_threads = 4;

        pthread_t *tid = alloca(num_threads * sizeof(*tid));
        strlist **out  = alloca(num_threads * sizeof(*out));
        warnx("Sorting through %ld tags with %d cpus.", tags->num, num_threads);

        /* Because we may have examined multiple tags files, it's very possible
         * for there to be duplicate tags. Sort the list and remove any. */
        qsort(vim_d, vimbuf->num, sizeof(*vim_d), &s_string_cmp);

        strlist *uniq = malloc(sizeof *uniq);
        *uniq = (strlist){
                .lst = nmalloc(vimbuf->num, sizeof *uniq->lst),
                .num = 1, .max = vimbuf->num
        };
        uniq_d[0] = vim_d[0];

        for (int i = 1; i < vimbuf->num; ++i)
                if (!string_eq(vim_d[i], vim_d[i-1]))
                        uniq_d[uniq->num++] = vim_d[i];

        /* Launch the actual search in separate threads, with each handling as
         * close to an equal number of tags as the math allows. */
        for (int i = 0; i < num_threads; ++i) {
                struct pdata *tmp = malloc(sizeof *tmp);
                int quot = (int)tags->num / num_threads;
                int num  = (i == num_threads - 1)
                              ? (int)(tags->num - ((num_threads - 1) * quot))
                              : quot;

                *tmp = (struct pdata){uniq, skip, equiv, lang, order, filename,
                                      tags->lst + (i * quot), num};

                if (pthread_create(tid + i, NULL, &do_tok_search, tmp) != 0)
                        err(1, "pthread_create failed");
        }

        /* Collect the threads. */
        for (int i = 0; i < num_threads; ++i)
                pthread_join(tid[i], (void **)(&out[i]));

        free_all(uniq->lst, uniq);
        unsigned total = 0, offset = 0;

        for (int T = 0; T < num_threads; ++T)
                total += out[T]->num;
        if (total == 0)
                errx(0, "No tags found in buffer.");

        /* Combine the returned data from all threads into one array, which is
         * then sorted and returned. */
        string **alldata = nmalloc(total, sizeof *alldata);
        strlist *ret     = malloc(sizeof *ret);
        *ret = (strlist){ alldata, total, total };

        for (int T = 0; T < num_threads; ++T) {
                if (out[T]->num > 0) {
                        memcpy(alldata + offset, out[T]->lst,
                               out[T]->num * sizeof(*out));
                        offset += out[T]->num;
                }
                free_all(out[T]->lst, out[T]);
        }

        qsort(alldata, total, sizeof(*alldata), &tag_cmp);

        return ret;
}

#define INIT_MAX ((data->num / 2) * 3)
#define cur_str (data->lst[i]->s)

static void *
do_tok_search(void *vdata)
{
        struct pdata *data = vdata;
        strlist *ret = malloc(sizeof *ret);
        *ret = (strlist){
                .lst = nmalloc(INIT_MAX, sizeof(string)),
                .num = 0, .max = INIT_MAX
        };

        for (int i = 0; i < data->num; ++i) {
                /* Skip empty lines and comments. */
                if (!cur_str[0] || cur_str[0] == '!')
                        continue;

                string name, match_file;

                /* The name is first, followed by two fields we don't need. */
                name.s         = strsep(&cur_str, "\t");
                name.len       = (cur_str - name.s - 1);
                match_file.s   = strsep(&cur_str, "\t");
                match_file.len = (cur_str - match_file.s - 1);
                cur_str = strchr(cur_str, '\t');

                char *tok, kind = '\0';
                string match_lang = { NULL, 0, 0 };

                /* Extract the 'kind' and 'language' fields. The former is the
                 * only one that is 1 character long, and the latter is prefaced. */
                while ((tok = strsep(&cur_str, "\t"))) {
                        if (tok[0] && !tok[1])
                                kind = *tok;
                        else if (strncmp(tok, "language:", 9) == 0)
                                match_lang = tostring(tok + 9);
                }

                if (!match_lang.s || !kind)
                        continue;
                string *name_p = &name;

                /* Prune tags. Include only those that are:
                 *    1) of a type in the `order' list,
                 *    2) of the correct language,
                 *    3) are not included in the `skip' list, and
                 *    4) are present in the current vim buffer.
                 * If invalid, just move on. */
                if ( in_order(data->equiv, data->order, &kind) &&
                     is_correct_lang(data->lang, &match_lang) &&
                    !skip_tag(data->skip, &name) &&
                     (string_eq(data->filename, &match_file) ||
                      bsearch(&name_p, data->vim_buf->lst, data->vim_buf->num,
                              sizeof(*data->vim_buf->lst), &s_string_cmp)))
                {
                        string *tmp = malloc(sizeof *tmp);
                        *tmp = (string){ name.s, name.len, kind };
                        add_to_list(ret, tmp);
                }
        }

        free(vdata);

#ifndef DOSISH
        pthread_exit(ret);
#else
        return ret;
#endif
}
