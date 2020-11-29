#include "queue.h"

int queue_check_params(queue_t **queue, queue_t *elem)
{
    if (queue == NULL)
    {
        //printf("queue is null");
        return 0;
    }
    if (elem == NULL)
    {
        //printf("elem is null");
        return 0;
    }
    return 1;
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
void queue_append(queue_t **queue, queue_t *elem)
{
    if (!queue_check_params(queue, elem))
        return;

    if (elem->next != NULL || elem->prev != NULL)
    {
        //printf("elem is on other queue\n");
        return;
    }

    if (queue[0]->next == NULL)
    {
        queue[0] = elem;
        queue[0]->next = queue[0];
        queue[0]->prev = queue[0];
        return;
    }

    queue_t *f = queue[0];
    queue_t *a = queue[0];
    while (a->next != f)
    {
        a = a->next;
    }
    a->next = elem;
    elem->prev = a;
    elem->next = f;
    queue[0]->prev = elem;
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove(queue_t **queue, queue_t *elem)
{
    if (!queue_check_params(queue, elem))
        return NULL;
    if (queue[0]->next == NULL)
    {
        //printf("queue is not empty\n");
        return NULL;
    }
    queue_t *f = queue[0];
    queue_t *a = queue[0];
    if (a == elem)
    {
        a->next = NULL;
        a->prev = NULL;
        return a;
    }
    a = a->next;
    // while (a->next != f)
    while (a != f)
    {
        if (a == elem)
        {
            queue_t *before = a->prev;
            queue_t *after = a->next;

            before->next = after;
            after->prev = before;

            // a->prev->next = a->next;
            // a->next->prev = a->prev;

            a->next = NULL;
            a->prev = NULL;

            return a;
        }
        a = a->next;
    }
    //printf("elem is not in the list\n");
    return NULL;
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size(queue_t *queue)
{
    if (queue == NULL)
        return 0;
    int i = 1;
    queue_t *f = &queue[0];
    queue_t *a = &queue[0];
    while (a->next != f)
    {
        a = a->next;
        i++;
    }
    return i;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca.
//
// Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print(char *name, queue_t *queue, void print_elem(void *))
{
    if (queue == NULL)
    {
        //printf("queue is null\n");
        return;
    }
    //printf("printing with %s\n", name);
    queue_t *f = &queue[0];
    queue_t *a = &queue[0];
    while (a->next != f)
    {
        print_elem((void *)a);
        a = a->next;
    }
    print_elem((void *)a);
}
