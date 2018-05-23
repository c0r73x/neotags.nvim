#include "neotags.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void remove_node(struct linked_list *list, struct Node *node);
static void free_data(struct Node *node, enum ll_datatypes dt);
typedef struct Node arsewipehole;

/* Note that contrary to its name, in this case `can_free_data` refers to the
 * string component of the struct string *, not the struct itself. The struct is
 * always free'd. The intention is to keep things adaptable. This macro could
 * change for different datatypes, and the field itself need not be boolean. */

#define CSS(NODE_) ((struct string *)(NODE_))
#if 0
#define FREE_DATA(LL_, NODE_)                    \
    do {                                         \
            if ((LL_)->can_free_data)            \
                    free(CSS((NODE_)->data)->s); \
            free((NODE_)->data);                 \
    } while (0)
#endif


struct linked_list *
new_list(enum ll_datatypes dt)
{
        struct linked_list *list = xmalloc(sizeof *list);
        list->head = list->tail = NULL;
        list->size = 0;
        list->dt   = dt;
        return list;
}


void
ll_add(struct linked_list *list, void *data)
{
        struct Node *node = xmalloc(sizeof *node);

        if (list->head != NULL)
                list->head->prev = node;
        if (list->tail == NULL)
                list->tail = node;

        node->data = data;
        node->prev = NULL;
        node->next = list->head;
        list->head = node;
        ++list->size;
}


void
ll_append(struct linked_list *list, void *data)
{
        struct Node *node = xmalloc(sizeof *node);

        if (list->tail != NULL)
                list->tail->next = node;
        if (list->head == NULL)
                list->head = node;

        node->data = data;
        node->prev = list->tail;
        node->next = NULL;
        list->tail = node;
        ++list->size;
}


void *
_ll_popat(struct linked_list *list, long index, enum ll_pop_type type)
{
        struct Node *node = ll_getnode_at_index(list, index);

        switch (type) {
        case DEL_ONLY:
                free_data(node, list->dt);
                remove_node(list, node);
                return NULL;
        case RET_ONLY:
                return node->data;
        case BOTH:
                ; void *data = node->data;
                remove_node(list, node);
                return data;
        default:
                errx(1, "Unreachable!\n");
        }
}


void
destroy_list(struct linked_list *list)
{
        if (list->size == 1) {
                struct Node *current;
                if (list->tail != NULL)
                        current = list->tail;
                else
                        current = list->head;
                free_data(current, list->dt);
                free(current);
        } else if (list->size > 1) {
                struct Node *current = list->head;
                do {
                        struct Node *tmp = current;
                        current = current->next;
                        free_data(tmp, list->dt);
                        free(tmp);
                } while (current != NULL);
        }

        free(list);
}


struct Node *
ll_getnode_at_index(struct linked_list *list, int64_t index)
{
        assert(list->size > 0);

        if (index == 0)
                return list->head;
        if (index == (-1))
                return list->tail;

        if (index < 0)
                index += list->size;
        if (index < 0 || index >= list->size)
                err(1, "index: %ld, size %u", index, list->size);
        /* else if (index == list->size)
                return NULL; */

        struct Node *current;

        /* If index is less than the size of the list divided by 2 (ie is in the
         * first half of the list) start the search from the head, otherwise
         * start from the tail. */
        if (index < ((list->size - 1) / 2)) {
                long x = 0;
                current = list->head;

                while (x++ != index)
                        current = current->next;
        } else {
                long x = list->size - 1;
                current = list->tail;

                while (x-- != index)
                        current = current->prev;
        }

        return current;
}


static void
remove_node(struct linked_list *list, struct Node *node)
{
        if (list->size == 1) {
                list->head = list->tail = NULL;
        } else if (node == list->head) {
                list->head = node->next;
                list->head->prev = NULL;
        } else if (node == list->tail) {
                list->tail = node->prev;
                list->tail->next = NULL;
        } else {
                node->prev->next = node->next;
                node->next->prev = node->prev;
        }
        --list->size;
        free(node);
}


/*============================================================================*/

#define CSS(NODE_) ((struct string *)(NODE_))


bool
ll_find_s_string(const struct linked_list *const list,
                 const char kind, const char *const name)
{
        for (struct Node *node = list->head; node != NULL; node = node->next)
                if (kind == CSS(node->data)->kind &&
                    streq(name, CSS(node->data)->s))
                        return true;
        return false;
}


bool
ll_find_string(const struct linked_list *const list, const char *const find)
{
        for (struct Node *node = list->head; node != NULL; node = node->next)
                if (streq(find, (char *)node->data))
                        return true;
        return false;
}


static void
free_data(struct Node *node, enum ll_datatypes dt)
{
        switch (dt) {
        case ST_STRING_FREE:
                free(CSS(node->data)->s);
        case ST_STRING_NOFREE: /* FALLTHROUGH */
        case SIMPLE:
                free(node->data);
                break;

        case ST_STRING_NOFREE_NOFREE:
        case SIMPLE_NOFREE:
                break;

        default:
                errx(1, "Invalid value for linked list datatype.");
        }
}
