#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <stdlib.h>

#define MY_TIMER_US 1000 //1000us

#if defined _WIN32

#include "timer/timer.h"

#else

#include <sys/time.h>
#include <signal.h>
#include <stdio.h>

// estrutura que define um tratador de sinal (deve ser global ou static)
static struct sigaction my_timer_action;

// estrutura de inicialização to timer
static struct itimerval my_timer_timer;

#endif
int my_timer_init(void (*_handler)(int), unsigned int _s, unsigned long _us);

#endif