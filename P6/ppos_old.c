#include "ppos.h"

int ppos_last_time = 0;
int ppos_timer_start = 1;
int ppos_task_id_counter;
int ppos_task_id_actual;
int ppos_interrupt_disable = 0;
char *ppos_task_stack;
task_t ppos_task_controller;
task_t ppos_task_last;
struct _ppos_scheduler ppos_scheduler;
unsigned int ppos_systime = 0;

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

    ppos_task_controller.user_task = 0;
    queue_t *q = (queue_t *)&ppos_task_controller;
    queue_append((queue_t **)&q, (queue_t *)&ppos_task_controller);

    // makecontext(&ppos_task_controller.context, ppos_init, 0);
    makecontext(&ppos_task_controller.context, (void *)(*main), 0);
    ppos_task_last = ppos_task_controller;

    //init of scheduler
    ppos_scheduler.current = -1;
    ppos_scheduler.user_tasks = 0;
    ppos_scheduler.queue = NULL;

    getcontext(&ppos_scheduler.dispatcher.context);
    stack = malloc(PPOS_TASK_STACK_SIZE);
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
    queue_t *q = (queue_t *)&ppos_task_controller;
    queue_append((queue_t **)&q, (queue_t *)task);
    ppos_scheduler.user_tasks++;

    return 1;
}
// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exitCode)
{
    my_timer_init(task_yield, 1, PREEMP_TIME_US);
    printf("task exit disabling interrupt\n");
    ppos_interrupt_disable = 1;
    //nao volta mais para main e sim para o scheduler
    task_t *t = NULL;
    t = &ppos_task_controller;
    while (t->id != ppos_task_id_actual)
    {
        t = t->next;
    }
    ppos_task_id_actual = t->prev->id;
    queue_t *q = queue_remove((queue_t **)&ppos_task_controller, (queue_t *)t);
    if (q == NULL)
    {
        printf("NULL on removing from queue\n");
    }
    else
    {
        printf("queue after removal: ");
        task_t *temp = &ppos_task_controller;
        printf(" %d < %d > %d ", temp->prev->id, temp->id, temp->next->id);
        temp = temp->next;
        while (temp != &ppos_task_controller)
        {
            printf(" %d < %d > %d ", temp->prev->id, temp->id, temp->next->id);
            temp = temp->next;
        }
        printf("\n");
    }

    //nao rolou liberar a memoria
    // free(t->context.uc_stack.ss_sp);
    ppos_scheduler.user_tasks--;
    //printf("dispatcher reenabling interrupt\n");
    ppos_interrupt_disable = 0;
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", t->id, ppos_systime - t->start_time, t->run_time, t->start_cicles);
    printf("Tasks left: %d\n", ppos_scheduler.user_tasks);
    my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
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
    task_t *next;
    while (1)
    {
        my_timer_init(task_yield, 1, PREEMP_TIME_US);
        if (ppos_scheduler.user_tasks <= 0)
        {
            ppos_interrupt_disable = 0;
            printf("Dispatcher going to main\n");
            my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
            swapcontext(&ppos_scheduler.dispatcher.context, &ppos_task_controller.context);
        }
        if ((ppos_interrupt_disable == 1) && (ppos_task_id_actual != 0))
        {
            next = &ppos_task_controller;
            while (next->id != ppos_task_id_actual)
            {
                next = next->next;
            }
            if (next->id != 0)
            {
                //printf("going to -1 %d\n", next->id);
                ppos_interrupt_disable = 0;
                my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
                swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
            }
        }
        else
        {
            //printf("dispatcher disabling interrupt\n");
            ppos_interrupt_disable = 1;
            // printf("actual id: %d\n", ppos_task_id_actual);
            task_t *current = (task_t *)&ppos_task_controller;
            // if (ppos_task_id_actual != 0)
            if (1)
            {
                //only user tasks can be preempted
                while (current->id != ppos_task_id_actual)
                {
                    current = current->next;
                }
                if (current->user_task != 1 && current->id != 0)
                {
                    //can only preempt user tasks
                    //printf("dispatcher reenabling interrupt\n");
                    ppos_interrupt_disable = 0;
                    //printf("going to 0 %d\n", current->id);
                    my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
                    swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
                }
                else if (current->quantum > 0)
                {
                    current->quantum--;
                    //printf("dispatcher reenabling interrupt\n");
                    ppos_interrupt_disable = 0;
                    //printf("going to 1 %d\n", current->id);
                    my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
                    swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
                }
                else
                {
                    current->run_time += ppos_systime - ppos_last_time;
                    int lesser_prio = 20;
                    int task_id;
                    current = (task_t *)&ppos_task_controller;
                    current = current->next;
                    while (current != &ppos_task_controller)
                    {
                        //task 0 has to be ignored
                        if ((current->prio_dinamic < lesser_prio) && (current->id != 0))
                        {
                            lesser_prio = current->prio_dinamic;
                            task_id = current->id;
                        }
                        if (current->next == NULL)
                        {
                            // current->next = ppos_task_controller.next;
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
                    //cant run task 0 because it is reserved for my sys
                    if (ppos_task_id_actual == 0)
                    {
                        next = next->next;
                        ppos_task_id_actual = next->id;
                    }
                    //update tasks dinamic prio
                    current = (task_t *)&ppos_task_controller;
                    current = current->next;
                    while (current != &ppos_task_controller)
                    {
                        // printf("task%d prio:%d\n", current->id, current->prio_dinamic);
                        if ((current->id != 0) && (current->id != ppos_task_id_actual))
                        {
                            current->prio_dinamic--;
                        }
                        if (current->next == NULL)
                        {

                            // current->next = ppos_task_controller.next;
                            break;
                        }
                        else
                        {
                            current = current->next;
                        }
                    }
                    // printf("dispatcher going to task %d", next->id);
                    //printf("dispatcher reenabling interrupt\n");
                    ppos_interrupt_disable = 0;
                    next->start_cicles++;
                    ppos_last_time = ppos_systime;
                    //printf("going to 2 %d\n", next->id);
                    my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
                    swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
                }
            }
        }
    }
    ppos_interrupt_disable = 0;
    printf("Dispatcher going to main\n");
    my_timer_init(task_yield, PREEMP_TIME_S, PREEMP_TIME_US);
    swapcontext(&ppos_scheduler.dispatcher.context, &ppos_task_controller.context);
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield()
{
    ppos_systime++;
    if (ppos_interrupt_disable)
    {
        // task_t *next = &ppos_task_controller;
        // while (next->id != ppos_task_id_actual)
        // {
        //     next = next->next;
        // }
        // swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
        return;
    }
    if (!ppos_timer_start)
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