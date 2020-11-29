#include "ppos.h"

int ppos_last_time = 0;
int ppos_timer_start = 1;
int ppos_task_id_counter;
int ppos_task_id_actual;
int ppos_interrupt_disable = 0;
char *ppos_task_stack;
task_t ppos_task_controller;
struct _ppos_scheduler ppos_scheduler;
unsigned int ppos_systime = 0;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init()
{
    // printf("\niniciando ppos_init\n");

    ppos_task_id_counter = 0;
    ppos_task_id_actual = 1;

    //init of scheduler
    ppos_scheduler.user_tasks = 0;
    ppos_scheduler.queue = NULL;

    getcontext(&ppos_scheduler.dispatcher.context);
    char *stack = malloc(PPOS_TASK_STACK_SIZE);
    if (stack)
    {
        ppos_scheduler.dispatcher.context.uc_stack.ss_sp = stack;
        ppos_scheduler.dispatcher.context.uc_stack.ss_size = PPOS_TASK_STACK_SIZE;
        ppos_scheduler.dispatcher.context.uc_stack.ss_flags = 0;
        ppos_scheduler.dispatcher.context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha: ");
        return;
    }

    ppos_scheduler.dispatcher.user_task = 0;
    makecontext(&ppos_scheduler.dispatcher.context, (void *)(*ppos_dispatcher), 0, NULL);

    // ppos_task_stack = malloc(PPOS_TASK_STACK_SIZE);
    // stack = malloc(PPOS_TASK_STACK_SIZE);
    // getcontext(&ppos_task_controller.context);
    // ppos_task_controller.id = 0;
    // if (stack)
    // {
    //     ppos_task_controller.context.uc_stack.ss_sp = stack;
    //     ppos_task_controller.context.uc_stack.ss_size = PPOS_TASK_STACK_SIZE;
    //     ppos_task_controller.context.uc_stack.ss_flags = 0;
    //     ppos_task_controller.context.uc_link = 0;
    // }
    // else
    // {
    //     perror("Erro na criação da pilha: ");
    //     return;
    // }

    // ppos_task_controller.user_task = 1;
    // ppos_task_controller.quantum = QUANTUM_INIT;
    // queue_t *q = (queue_t *)&ppos_task_controller;
    // queue_append((queue_t **)&q, (queue_t *)&ppos_task_controller);
    // makecontext(&ppos_task_controller.context, (void *)(*main), 0);
    // ppos_scheduler.user_tasks++;
    // task_create(&ppos_task_controller, (void *)(*main), NULL);

    // task_t *task = &ppos_task_controller;
    // task->id = ppos_task_id_counter;
    // getcontext(&task->context);
    // stack = malloc(PPOS_TASK_STACK_SIZE);
    // if (stack)
    // {
    //     task->context.uc_stack.ss_sp = stack;
    //     task->context.uc_stack.ss_size = PPOS_TASK_STACK_SIZE;
    //     task->context.uc_stack.ss_flags = 0;
    //     task->context.uc_link = 0;
    // }
    // else
    // {
    //     perror("Erro na criação da pilha: ");
    //     return;
    // }
    // makecontext(&task->context, (void *)(*main), 0, NULL);
    // task->prio_static = 0;
    // task->prio_dinamic = 0;
    // task->quantum = QUANTUM_INIT;
    // task->user_task = 1;
    // task->start_time = ppos_systime;
    // task->start_cicles = 0;
    // task->run_time = 0;
    // queue_t *q = (queue_t *)&ppos_task_controller;
    // queue_append((queue_t **)&q, (queue_t *)task);
    // ppos_scheduler.user_tasks++;
    // task_create(&ppos_task_controller, ppos_fake_main, NULL);
    task_create(&ppos_task_controller, main, NULL);

    if (ppos_timer_start)
    {
        my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
    }
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
    task->prio_static = 0;
    task->prio_dinamic = 0;
    task->quantum = QUANTUM_INIT;
    task->user_task = 1;
    task->start_time = ppos_systime;
    task->start_cicles = 0;
    task->run_time = 0;
    task->joined_id = -1;
    task->exit_value = 0;
    queue_t *q = (queue_t *)&ppos_task_controller;
    queue_append((queue_t **)&q, (queue_t *)task);
    ppos_scheduler.user_tasks++;

    return 1;
}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exitCode)
{
    //gambiarra
    // if (ppos_task_id_actual == 1)
    // {
    //     return;
    // }
    ppos_interrupt_disable = 1;

    //update tasks exit values and joined values
    task_t *temp = &ppos_task_controller;
    if (temp->joined_id == ppos_task_id_actual)
    {
        temp->exit_value = exitCode;
        temp->joined_id = -1;
    }
    temp = temp->next;
    while (temp != &ppos_task_controller)
    {
        if (temp->joined_id == ppos_task_id_actual)
        {
            temp->exit_value = exitCode;
            temp->joined_id = -1;
        }
        temp = temp->next;
    }

    task_t *t = NULL;
    t = &ppos_task_controller;
    while (t->id != ppos_task_id_actual)
    {
        t = t->next;
    }
    ppos_task_id_actual = t->prev->id;

    //ultra gambiarra em acao
    //begin
    // task_t *q = (task_t *)queue_remove((queue_t **)&ppos_task_controller, (queue_t *)t);
    task_t *p = t->prev;
    task_t *n = t->next;
    t->prev->next = n;
    t->next->prev = p;
    t->next = NULL;
    t->prev = NULL;
    //end
    task_t *q = t;
    if (q == NULL)
    {
        printf("NULL on removing from queue\n");
    }
    // else
    // {
    //     printf("queue after removal: ");
    //     task_t *temp = &ppos_task_controller;
    //     printf(" %d < %d > %d ", temp->prev->id, temp->id, temp->next->id);
    //     temp = temp->next;
    //     while (temp != &ppos_task_controller)
    //     {
    //         printf(" %d < %d > %d ", temp->prev->id, temp->id, temp->next->id);
    //         temp = temp->next;
    //     }
    //     printf("\n");
    // }
    //nao rolou liberar a memoria
    // free(t->context.uc_stack.ss_sp);
    ppos_scheduler.user_tasks--;
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", t->id, ppos_systime - t->start_time, t->run_time, t->start_cicles);
    ppos_interrupt_disable = 0;
    // printf("Tasks left: %d\n", ppos_scheduler.user_tasks);
    swapcontext(&t->context, &ppos_scheduler.dispatcher.context);
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    task_t *t = &ppos_task_controller;
    while (t->id != task->id)
    {
        t = t->next;
    }
    task_t *t2 = &ppos_task_controller;
    while (t2->id != ppos_task_id_actual)
    {
        t2 = t2->next;
    }
    printf("\nindo de %d id para %d id\n", ppos_task_id_actual, task->id);
    // printf("\nterminando task_switch\n");
    ppos_task_id_actual = task->id;
    swapcontext(&t2->context, &task->context);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
    return ppos_task_id_actual;
}

void ppos_dispatcher()
{
    while (ppos_scheduler.user_tasks > 0)
    {
        //gambiarra
        // if (ppos_scheduler.user_tasks == 1)
        // {
        //     printf("Only one task left\n");
        //     break;
        // }
        ppos_interrupt_disable = 1;
        task_t *current = (task_t *)&ppos_task_controller;
        while (current->id != ppos_task_id_actual)
        {
            current = current->next;
        }
        if (current->user_task != 1)
        {
            //can only preempt user tasks
            ppos_interrupt_disable = 0;
            swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
        }
        else if (current->quantum > 0)
        {
            current->quantum--;
            ppos_interrupt_disable = 0;
            swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
        }
        else
        {
            current->run_time += ppos_systime - ppos_last_time;
            int lesser_prio = 20;
            int task_id = 1;
            current = (task_t *)&ppos_task_controller;
            if ((current->prio_dinamic < lesser_prio) && (current->user_task != 0))
            {
                lesser_prio = current->prio_dinamic;
                task_id = current->id;
            }
            current = current->next;
            while (current != &ppos_task_controller)
            {
                if ((current->prio_dinamic < lesser_prio) && (current->user_task != 0))
                {
                    lesser_prio = current->prio_dinamic;
                    task_id = current->id;
                }
                if (current->next == NULL)
                {
                    //bug in the code
                    break;
                }
                else
                {
                    current = current->next;
                }
            }
            //going to next task
            task_t *next = &ppos_task_controller;
            while (next->id != task_id)
            {
                next = next->next;
            }
            ppos_task_id_actual = next->id;
            next->prio_dinamic = next->prio_static;
            //update tasks dinamic prio
            current = (task_t *)&ppos_task_controller;
            if ((current->user_task != 0) && (current->id != ppos_task_id_actual))
            {
                current->prio_dinamic--;
            }
            current = current->next;
            while (current != &ppos_task_controller)
            {
                if ((current->user_task != 0) && (current->id != ppos_task_id_actual))
                {
                    current->prio_dinamic--;
                }
                if (current->next == NULL)
                {
                    break;
                }
                else
                {
                    current = current->next;
                }
            }
            next->start_cicles++;
            ppos_last_time = ppos_systime;
            // printf("Dispather choose %d\n", next->id);
            ppos_interrupt_disable = 0;
            swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
        }
    }
    ppos_interrupt_disable = 0;
    printf("Going back to main\n");
    swapcontext(&ppos_scheduler.dispatcher.context, &ppos_task_controller.context);
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield()
{
    ppos_systime++;
    if (ppos_interrupt_disable)
    {
        return;
    }
    //this is the function called when timer interrupt
    // printf("task yield\n");
    if (ppos_timer_start == 0)
    {
        ppos_timer_start = 1;
        my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
    }
    task_t *current = &ppos_task_controller;
    while (current->id != ppos_task_id_actual)
    {
        current = current->next;
    }
    swapcontext(&current->context, &ppos_scheduler.dispatcher.context);
}
// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio(task_t *task, int prio)
{
    if (task == NULL)
    {
        task_t *t2 = &ppos_task_controller;
        while (t2->id != ppos_task_id_actual)
        {
            t2 = t2->next;
        }
        t2->prio_static = prio;
        return;
    }
    task->prio_static = prio;
    task->prio_dinamic = prio;
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
    if (task == NULL)
    {
        task_t *t2 = &ppos_task_controller;
        while (t2->id != ppos_task_id_actual)
        {
            t2 = t2->next;
        }
        return t2->prio_static;
    }
    return task->prio_static;
}

// retorna o relógio atual (em milisegundos)
unsigned int systime()
{
    return ppos_systime;
}

// a tarefa corrente aguarda o encerramento de outra task
int task_join(task_t *task)
{
    int found = 0;
    task_t *t2 = &ppos_task_controller;
    t2 = t2->next;
    while (t2 != &ppos_task_controller)
    {
        if (t2 == task)
            found = 1;
        t2 = t2->next;
    }
    if (found == 1)
    {
        task_t *t1 = &ppos_task_controller;
        while (t1->id != ppos_task_id_actual)
        {
            t1 = t1->next;
        }
        t1->joined_id = task->id;
        while (t1->joined_id != -1)
            ;
        return t1->exit_value;
    }
    return -1;
}