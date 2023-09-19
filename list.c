#include <stdlib.h>
#include "list.h"

List* list_create() {
    List* list = malloc(sizeof(List));
    if (list == NULL) 
    {
        return NULL;
    }

    list->array = NULL;
    list->size = 0;
    list->capacity = 0;

    return list;
}

void list_destroy(List* list) {
    if (list != NULL) 
    {
        if(list->array != NULL)
            free(list->array);
        
        free(list);
    }
}
void list_free_elements(List* list)
{
    while(list->size > 0)
    {
        void* el = list_pop(list);
        free(el);
    }
}
void list_add(List* list, void* value)
{
    if(list->capacity <= list->size)
    {
        list->capacity = list->capacity == 0 ? 1 : list->capacity * 2;
        list->array = realloc(list->array,list->capacity * sizeof(void*));
    }

    ((void**)list->array)[list->size] = value;
    list->size++;

}

void list_sort(List* list, int (*cmp)(void *, void *))
{
    int i, j;
    for(i=1;i<list->size;i++)
    {
        void * v = list->array[i];
        j = i-1;


        while(j >= 0 && cmp(list->array[j],v) == 1)
        {
            list->array[j + 1] = list->array[j];
            j = j -1;
        }
        list->array[j+1] = v;
    }
}
void* list_pop(List* list)
{
    if(list->size == 0)
    {
        return NULL;
    }

    void* value = ((void**)list->array)[list->size - 1];
    list->size--;

    if (list->capacity > 1 && list->capacity > 4 * list->size)
    {
         
        list->capacity = list->capacity / 2;
        list->array = realloc(list->array, list->capacity * sizeof(void*));
    }

    return value;
}
void* list_peek(List* list)
{
    if(list->size == 0)
    {
        return NULL;
    }

    void* value = ((void**)list->array)[list->size - 1];

    return value;
}
void* list_peek_index(List* list,int index)
{
    if(list->size == 0 || index > list->size)
    {
        return NULL;
    }

    void* value = ((void**)list->array)[index];

    return value;
}
void* list_peek_first(List* list)
{
    if(list->size == 0)
    {
        return NULL;
    }

    void* value = ((void**)list->array)[0];

    return value;
}
void list_clear(List* list)
{
    list->size = 0;
    list->capacity = 0;
    free(list->array);
    list->array = NULL;
}
void list_reverse(List* list)
{
    for (int i = 0; i < list->size / 2; i++)
    {
        void* temp = ((void**)list->array)[i];
        ((void**)list->array)[i] = ((void**)list->array)[list->size - i - 1];
        ((void**)list->array)[list->size - i - 1] = temp;
    }
}