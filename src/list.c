#include "mf/list.h"

void list_append(list **head, list *n)
{
        n->next = 0;
        if ((*head) && (*head)->last) {
                (*head)->last->next = n;
                (*head)->last = n;
                return;
        }

        list **current = head;
        while (*current) {
                current = &(*current)->next;
        }

        *current = n;
        (*head)->last = n;
}
