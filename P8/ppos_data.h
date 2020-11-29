// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#if defined _WIN32
#include "ucontext.h"
#else
#include <ucontext.h> // biblioteca POSIX de trocas de contexto
#endif

#include "queue.h" // biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
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
  unsigned int start_time;
  unsigned int start_cicles;
  unsigned int run_time;
  unsigned int joined_id;
  int exit_value;
} task_t;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t;

#endif
