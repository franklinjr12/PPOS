#include "ppos.h"
#ifdef _cplusplus
extern "C"
{
#endif

    int ppos_timer_start = 0;
    int ppos_task_id_counter;
    int ppos_task_id_actual;
    char *ppos_task_stack;
    task_t ppos_task_controller;
    task_t ppos_task_last;
    struct _ppos_scheduler ppos_scheduler;
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
        queue_t *q = (queue_t *)&ppos_task_controller;
        queue_append((queue_t **)&q, (queue_t *)task);
        ppos_scheduler.user_tasks++;
        // printf("\nterminando task_create\n");

        return 1;
    }
    // Termina a tarefa corrente, indicando um valor de status encerramento
    void task_exit(int exitCode)
    {
        // // printf("\niniciando task_exit\n");
        // task_t *t = NULL;
        // // printf("\nppos_task_controller.id = %d\n", ppos_task_controller.id);
        // t = &ppos_task_controller;
        // while (t->id != ppos_task_id_actual)
        // {
        //     t = t->next;
        // }
        // // printf("\nindo de %did para %did\n", t->id, ppos_task_controller.id);
        // // printf("\nterminando task_exit\n");
        // swapcontext(&t->context, &ppos_task_controller.context);

        //nao volta mais para main e sim para o scheduler
        task_t *t = NULL;
        t = &ppos_task_controller;
        while (t->id != ppos_task_id_actual)
        {
            t = t->next;
        }
        ppos_task_id_actual = t->prev->id;
        queue_remove((queue_t **)&ppos_task_controller, (queue_t *)t);
        //nao rolou liberar a memoria
        // free(t->context.uc_stack.ss_sp);
        ppos_scheduler.user_tasks--;
        swapcontext(&t->context, &ppos_scheduler.dispatcher.context);

        // printf("\nterminando task_exit\n");
    }

    // alterna a execução para a tarefa indicada
    int task_switch(task_t *task)
    {
        // printf("\niniciando task_switch\n");

        // int i = 0;
        // task_t *t = &ppos_task_controller;
        // while (i != task->id)
        // {
        //     t = t->next;
        //     i++;
        // }
        // task_t *t2 = &ppos_task_controller;
        // i = 0;
        // while (i != ppos_task_id_actual)
        // {
        //     t2 = t2->next;
        //     i++;
        // }

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

    void ppos_dispatcher()
    {
        while (ppos_scheduler.user_tasks > 0)
        {
            printf("actual id: %d\n", ppos_task_id_actual);
            task_t *current = (task_t *)&ppos_task_controller;
            if (ppos_task_id_actual != 0)
            {
                //only user tasks can be preempted
                while (current->id != ppos_task_id_actual)
                {
                    current = current->next;
                }
                if (current->user_task != 1)
                {
                    //can only preempt user tasks
                    return;
                }
                if (current->quantum > 0)
                {
                    current->quantum--;
                    swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
                }
            }

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
                current = current->next;
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
                printf("task%d prio:%d\n", current->id, current->prio_dinamic);
                if ((current->id != 0) && (current->id != ppos_task_id_actual))
                {
                    current->prio_dinamic--;
                }
                current = current->next;
            }
            printf("dispatcher: going to task %d", next->id);
            swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
        }
        swapcontext(&ppos_scheduler.dispatcher.context, &ppos_task_controller.context);
    }

    // libera o processador para a próxima tarefa, retornando à fila de tarefas
    // prontas ("ready queue")
    void task_yield()
    {
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

    //USED IN OLDER VERSIONS
    // void ppos_dispatcher()
    // {
    //     while (ppos_scheduler.user_tasks > 0)
    //     {
    //         // task_t *current = &ppos_task_controller;
    //         // while (current->id != ppos_task_id_actual)
    //         // {
    //         //     current = current->next;
    //         // }
    //         // //going to next task
    //         // current = current->next;
    //         // ppos_task_id_actual = current->id;
    //         // //cant run task 0 because it is reserved for my sys
    //         // if (ppos_task_id_actual == 0)
    //         // {
    //         //     current = current->next;
    //         //     ppos_task_id_actual = current->id;
    //         // }
    //         // swapcontext(&ppos_scheduler.dispatcher.context, &current->context);
    //         // // task_switch(current);

    //         int lesser_prio = 20;
    //         int task_id;
    //         // task_t *current = (task_t *)&ppos_task_controller.next;
    //         task_t *current = (task_t *)&ppos_task_controller;
    //         current = current->next;
    //         while (current != &ppos_task_controller)
    //         {
    //             //task 0 has to be ignored
    //             if ((current->prio_static < lesser_prio) && (current->id != 0))
    //             {
    //                 lesser_prio = current->prio_static;
    //                 task_id = current->id;
    //             }
    //             current = current->next;
    //         }

    //         //going to next task
    //         task_t *next = &ppos_task_controller;
    //         while (next->id != task_id)
    //         {
    //             next = next->next;
    //         }
    //         ppos_task_id_actual = current->id;
    //         next->prio_dinamic = next->prio_static;
    //         //cant run task 0 because it is reserved for my sys
    //         if (ppos_task_id_actual == 0)
    //         {
    //             current = current->next;
    //             ppos_task_id_actual = current->id;
    //         }
    //         //update tasks dinamic prio
    //         // current = (task_t *)&ppos_task_controller.next;
    //         current = (task_t *)&ppos_task_controller;
    //         current = current->next;
    //         while (current != &ppos_task_controller)
    //         {
    //             if ((current->id != 0) || (current->id != ppos_task_id_actual))
    //             {
    //                 current->prio_dinamic--;
    //             }
    //             current = current->next;
    //         }
    //         swapcontext(&ppos_scheduler.dispatcher.context, &next->context);
    //     }
    //     // printf("Fim dispatcher");
    //     swapcontext(&ppos_scheduler.dispatcher.context, &ppos_task_controller.context);
    // }

#ifdef _cplusplus
}
#endif
