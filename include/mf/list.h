#ifndef mf_list
#define mf_list

typedef struct list {
        void *data;
        struct list *next;
        struct list *last;
} list;

void list_append(list **head, list *n);

#endif
