#include "ppos.h"

int ppos_task_id_counter;
int ppos_task_id_actual;
char *ppos_task_stack;
task_t ppos_task_controller;
task_t ppos_task_last;
// static int ppos_task_id_counter;
// static int ppos_task_id_actual;
// static char *ppos_task_stack;
// static task_t ppos_task_controller;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init()
{
    // printf("\niniciando ppos_init\n");

    ppos_task_id_counter = 0;
    ppos_task_id_actual = 0;
    // ppos_task_stack = malloc(PPOS_TASK_STACK_SIZE);
    char *stack = malloc(PPOS_TASK_STACK_SIZE);
    getcontext(&ppos_task_controller.context);
    ppos_task_controller.id = 0;
    if (stack)
    {
        ppos_task_controller.context.uc_stack.ss_sp = stack;
        ppos_task_controller.context.uc_stack.ss_size = PPOS_TASK_STACK_SIZE;
        ppos_task_controller.context.uc_stack.ss_flags = 0;
        ppos_task_controller.context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha: ");
        return;
    }

    queue_t *q = (queue_t *)&ppos_task_controller;
    queue_append((queue_t **)&q, (queue_t *)&ppos_task_controller);

    // makecontext(&ppos_task_controller.context, ppos_init, 0);
    makecontext(&ppos_task_controller.context, (void *)(*main), 0);
    ppos_task_last = ppos_task_controller;
    // printf("\nterminando ppos_init\n");
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create(task_t *task,               // descritor da nova tarefa
                void (*start_func)(void *), // funcao corpo da tarefa
                void *arg)                  // argumentos para a tarefa
{
    // printf("\niniciando task_create\n");

    task->id = ++ppos_task_id_counter;
    // ppos_task_stack = malloc(PPOS_TASK_STACK_SIZE);
    char *stack;
    getcontext(&task->context);
    stack = malloc(PPOS_TASK_STACK_SIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = PPOS_TASK_STACK_SIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha: ");
        return 0;
    }

    makecontext(&task->context, (void *)(*start_func), 1, arg);

    queue_t *q = (queue_t *)&ppos_task_controller;
    queue_append((queue_t **)&q, (queue_t *)task);

    // printf("\nterminando task_create\n");

    return 1;
}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exitCode)
{
    // printf("\niniciando task_exit\n");
    //como voltar a task main???????????
    task_t *t = NULL;
    // printf("\nppos_task_controller.id = %d\n", ppos_task_controller.id);
    t = &ppos_task_controller;
    while (t->id != ppos_task_id_actual)
    {
        t = t->next;
    }
    // printf("\nindo de %did para %did\n", t->id, ppos_task_controller.id);
    // printf("\nterminando task_exit\n");
    swapcontext(&t->context, &ppos_task_controller.context);
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    // printf("\niniciando task_switch\n");

    int i = 0;
    task_t *t = &ppos_task_controller;
    while (i != task->id)
    {
        t = t->next;
        i++;
    }
    task_t *t2 = &ppos_task_controller;
    i = 0;
    while (i != ppos_task_id_actual)
    {
        t2 = t2->next;
        i++;
    }
    // printf("\nindo de %d id para %d id\n", ppos_task_id_actual, task->id);
    // printf("\nterminando task_switch\n");
    ppos_task_id_actual = task->id;
    swapcontext(&t2->context, &task->context);

    // ppos_task_id_actual = task->id;
    // task_t temp = ppos_task_last;
    // ppos_task_last = *task;
    // swapcontext(&temp.context, &task->context);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
    return ppos_task_id_actual;
}