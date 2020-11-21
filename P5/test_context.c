#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "my_timer.h"

#define PPOS_TASK_STACK_SIZE 32000

typedef struct task_t
{
    struct task_t *prev, *next; // ponteiros para usar em filas
    int id;                     // identificador da tarefa
    ucontext_t context;         // contexto armazenado da tarefa
                                // ... (outros campos serão adicionados mais tarde)
    int prio_static;
    int prio_dinamic;
    int quantum;
    int user_task; //if equals 1 then it is an user task and can be preempted
} task_t;

int create_task(task_t *task,               // descritor da nova tarefa
                void (*start_func)(void *), // funcao corpo da tarefa
                void *arg)
{
    int ret;
    char *stack;
    stack = (char *)malloc(sizeof(char) * PPOS_TASK_STACK_SIZE);
    ret = getcontext(&task->context);
    if (ret)
    {
        return ret;
    }
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
        return 1;
    }

    makecontext(&task->context, (void *)(*start_func), 1, arg);
    return 0;
}

void body(void *arg)
{
    char *s = (char *)arg;
    printf("%s init\n", s);
    for (int i = 0; i < 5; i++)
    {
        printf("%s %d\n", s, i);
    }
    printf("%s end\n", s);
}

int main(int argc, char *argv[])
{
    printf("Testing ucontext and my_timer\n");

    int err;

    task_t tmain;
    char smain[] = "taskmain";
    err = create_task(&tmain, (void *)(*main), (void *)smain);
    if (err)
    {
        printf("error ocurred code: %d\n", err);
        exit(0);
    }

    task_t t1;
    char s1[] = "task1";
    err = create_task(&t1, body, (void *)s1);
    if (err)
    {
        printf("error ocurred code: %d\n", err);
        exit(0);
    }

    task_t t2;
    char s2[] = "task2";
    err = create_task(&t2, body, (void *)s2);
    if (err)
    {
        printf("error ocurred code: %d\n", err);
        exit(0);
    }

    err = swapcontext(&tmain.context, &t1.context);
    if (err)
    {
        printf("error ocurred code: %d\n", err);
        exit(0);
    }

    err = swapcontext(&tmain.context, &t2.context);
    if (err)
    {
        printf("error ocurred code: %d\n", err);
        exit(0);
    }

    printf("end main\n");
    return 0;
}
