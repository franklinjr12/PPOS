// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste da gestão básica de tarefas

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define MAXTASK 5

task_t task;
task_t t1, t2, t3, t4;

// corpo das threads
void BodyTask(void *arg)
{
   printf("Estou na tarefa %5d\n", task_id());
   task_exit(0);
}

void b1(void *arg)
{
   printf("\nbody 1\n");
   printf("Estou na tarefa %5d\n", task_id());
   task_switch(&t3);
}
void b2(void *arg)
{
   printf("\nbody 2\n");
   printf("Estou na tarefa %5d\n", task_id());
   task_switch(&t4);
}
void b3(void *arg)
{
   printf("\nbody 3\n");
   printf("Estou na tarefa %5d\n", task_id());
   task_exit(0);
}
void b4(void *arg)
{
   printf("\nbody 4\n");
   printf("Estou na tarefa %5d\n", task_id());
   task_switch(&t1);
}

int main(int argc, char *argv[])
{
   int i;

   printf("main: inicio\n");

   ppos_init();

   // task_create(&t1, b1, NULL);
   // task_create(&t2, b2, NULL);
   // task_create(&t3, b3, NULL);
   // task_create(&t4, b4, NULL);

   // task_switch(&t2);

   // printf("\nend main\n");

   // return 0;

   // cria MAXTASK tarefas, ativando cada uma apos sua criacao
   for (i = 0; i < MAXTASK; i++)
   {
      task_create(&task, BodyTask, NULL);
      task_switch(&task);
   }

   printf("main: fim\n");

   exit(0);
}
