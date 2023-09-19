typedef struct {
    void **array;
    int size;
    int capacity;
} List;

List* list_create(void);
void list_destroy(List *list);
void list_add(List *list, void *value);
void list_sort(List *list, int (*cmp)(void *, void *));
void *list_pop(List *list);
void *list_peek(List *list);
void list_clear(List *list);
void list_reverse(List *list);
void* list_peek_first(List* list);
void list_free_elements(List* list);
void* list_peek_index(List* list,int index);