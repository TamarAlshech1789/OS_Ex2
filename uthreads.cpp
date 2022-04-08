#include "uthreads.h"
#include "definitions.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>
#include <iostream>
#include <queue>
#include <list>
#include <map>
using namespace std;

typedef struct Thread{

    int tid;
    int quantumRunTime;
    char stack[STACK_SIZE];
    sigjmp_buf env;
    thread_entry_point entry_point;
}Thread;

int runningThread;
map<int, Thread*> threads;
list<int> ready_threads;
list<int> block_threads;
priority_queue<int, vector<int>, greater<int> > ID_unused;

int total_time;
int quantum;
struct itimerval timer;
struct sigaction sa = {0};
sigset_t set;


#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#endif

//print errors in program
void print_system_error(string errorMsg)
{
    cerr << "system error: " << errorMsg << endl;
}
void print_thread_library_error(string errorMsg)
{
    cerr << "thread library error: " << errorMsg << endl;
}

//free all memory the thread library used
void delete_memory()
{
    for ( auto it = threads.begin(); it != threads.end(); it++)
        delete it->second;
        //free(it->second);
    threads.clear();
}

void timer_handler(int sig)
{
    //move_current_to_ready(); // save context and add id to ready list
    //round_robin(); // find next ready thread, remove from ready list and update running

}

//block st of signal
void block_signal(int signo)
{
    //initialize set to be empty signal set
    if(sigemptyset(&set))
    {
        print_system_error("sigemptyset failed.");
        delete_memory();
        exit(1);
    }

    //add signal to set
    if(sigaddset(&set, signo))
    {
        print_system_error("sigaddset failed.");
        delete_memory();
        exit(1);
    }
}

//block timer
void block_timer()
{
    block_signal(SIGVTALRM);

    //block the timer
    if(sigprocmask(SIG_BLOCK, &set, NULL))
    {
        print_system_error("block timer failed.");
        delete_memory();
        exit(1);
    }
}


//unblock timer
void unblock_timer()
{
    block_signal(SIGVTALRM);

    //block the timer
    if(sigprocmask(SIG_UNBLOCK, &set, NULL))
    {
        print_system_error("block timer failed.");
        delete_memory();
        exit(1);
    }
}

/*
 * Initializes the thread library
 */
int uthread_init(int quantum_usecs)
{
    //check quantum_usecs input
    if( quantum_usecs < 0) {
        print_thread_library_error("quantum_usecs needs to be non-negative!");
        return -1;
    }

    //initialize quantum
    quantum = quantum_usecs;
    total_time = 1;

    //set virtual timer
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        print_system_error("sigaction failed for installing virtual timer.");
        exit(1);
    }
    //initialize timer
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_usec = quantum;
    timer.it_value.tv_sec = 0;
    //start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        print_system_error("setitimer failed for starting virtual timer.");
        exit(1);
    }

    //create main thread
    //Thread* main_thread = (Thread*) malloc(sizeof(Thread));
    Thread* main_thread = new Thread;
    threads[0] = main_thread;
    main_thread->tid = 0;
    main_thread->quantumRunTime = 1;
    runningThread = main_thread->tid;

    //init ID_unsused list
    for(int i = 1; i <=  MAX_THREAD_NUM; i++)
        ID_unused.push(i);

    return 0;

}

/*
 * Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void)
*/
int uthread_spawn(thread_entry_point entry_point)
{
    block_timer();
    if (threads.size() == MAX_THREAD_NUM)
    {
        print_thread_library_error("reached maximum threads.");
        unblock_timer();
        return -1;
    }

    if (entry_point == nullptr)
    {
        print_thread_library_error("entry_point is null.");
        unblock_timer();
        return -1;
    }

    //define new thread
    Thread* new_thread = new Thread();
    int tid = ID_unused.top();
    new_thread->tid = tid;
    new_thread->entry_point = entry_point;
    address_t sp = (address_t)new_thread->stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t)new_thread;
    if (sigsetjmp(new_thread->env, 1))
    {
        print_system_error("sigsetjmp failed.");
        exit(1);
    }
    (new_thread->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (new_thread->env->__jmpbuf)[JB_PC] = translate_address(pc);
    if(sigemptyset(&new_thread->env->__saved_mask))
    {
        print_system_error("sigemptyset failed.");
        exit(1);
    }

    //save new thread to threads list and remove tid from undused_ID list
    ID_unused.pop();
    threads[tid] = new_thread;
    ready_threads.push_back(tid);

    unblock_timer();

    return tid;
}
