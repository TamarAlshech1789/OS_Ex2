#include "uthreads.h"
#include "definitions.h"
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

Schedule scehuler;


int uthread_init(int quantum_usecs)
{
    struct sigaction sa = {.sa_handler = sa_handler};

    if( quantum_usecs <= 0)
    {
        //ERROR!!
        return -1;
    }

    scehuler.quantumTime = quantum_usecs;
    for(int i = 1; i <=  MAX_THREAD_NUM; i++)
        scehuler.ID_unused.push(i);

    Thread threadMain{0, State::RUNNING};
    threadMain.quantumRunTime.tv_sec = 1;
    threadMain.quantumRunTime.tv_usec = 0;



}

