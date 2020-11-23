#include "my_timer.h"

#if defined _unix_

int my_timer_init(void (*_handler)(int), unsigned int _s, unsigned long _us)
{
    // registra a ação para o sinal de timer SIGALRM
    my_timer_action.sa_handler = _handler;
    sigemptyset(&my_timer_action.sa_mask);
    my_timer_action.sa_flags = 0;
    if (sigaction(SIGALRM, &my_timer_action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    my_timer_timer.it_value.tv_usec = _us;    // primeiro disparo, em micro-segundos
    my_timer_timer.it_value.tv_sec = _s;      // primeiro disparo, em segundos
    my_timer_timer.it_interval.tv_usec = _us; // disparos subsequentes, em micro-segundos
    my_timer_timer.it_interval.tv_sec = _s;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer(ITIMER_REAL, &my_timer_timer, 0) < 0)
    {
        perror("Erro em setitimer: ");
        exit(1);
    }

    return 0;
}

#else
#include <stdio.h>

int my_timer_init(void (*_handler)(int), unsigned int _s, unsigned long _us)
{
    int s = 1000 * _s;
    int us = _us / 1000;
    void (*handler)(void) = (void *)(*_handler);
    start_timer(s + us, handler);
}

#endif //defined LINUX