#include <stdio.h>
#include "queue.h"

typedef struct queue_int
{
    struct queue_int *prev; // aponta para o elemento anterior na fila
    struct queue_int *next; // aponta para o elemento seguinte na fila
    int elem;
} queue_int;

void queue_int_print(queue_int **_q)
{
    printf("\nprinting queue_int:\n");
    if (_q == NULL)
    {
        printf("queue is empty\n");
        return;
    }
    queue_int *f = _q[0];
    queue_int *a = f;
    if (_q[0]->prev == NULL || _q[0]->next == NULL)
    {
        printf(" 0<%d>0\n", a->elem);
        return;
    }
    while (a->next != f)
    {
        printf(" %d<%d>%d", a->prev->elem, a->elem, a->next->elem);
        a = a->next;
    }
    printf(" %d<%d>%d\n", a->prev->elem, a->elem, a->next->elem);
}
void queue_int_print_elem(void *ptr)
{
    if (ptr == NULL)
    {
        printf("queue is empty\n");
        return;
    }
    queue_int *q = (queue_int *)ptr;
    printf("%d<%d>%d ", q->prev->elem, q->elem, q->next->elem);
}

int main(void)
{
    printf("init\n");

    queue_int *first = NULL;

    const int num_elem = 4;
    queue_int queue[num_elem];
    for (int i = 0; i < num_elem; i++)
    {
        queue[i].elem = i;
        queue[i].next = NULL;
        queue[i].prev = NULL;
    }

    first = &queue[0];

    printf("testing adding elements\n");
    for (int i = 0; i < num_elem; i++)
    {
        queue_t *actual = (queue_t *)&queue[i];
        queue_append((queue_t **)&first, actual);
        queue_int_print((queue_int **)&queue[0]);
    }
    printf("size of queue: %d\n", queue_size((queue_t *)queue));

    printf("testing removing elements\n");
    queue_int *r = (queue_int *)queue_remove((queue_t **)&queue[0], (queue_t *)&queue[1]);
    queue_int_print((queue_int **)&queue[0]);
    printf("size of queue: %d\n", queue_size((queue_t *)queue));
    if (r == NULL)
    {
        printf("error removing elem\n");
    }
    else
        queue_int_print((queue_int **)&r);

    printf("testing printing elements\n");
    queue_print("queue_int", (queue_t *)queue, queue_int_print_elem);

    printf("\nprogram ended correctly\n");
    return 0;
}