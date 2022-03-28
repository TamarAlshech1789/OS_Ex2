#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <iostream>
#include <queue>
#include <list>
#include <sys/time.h>
using namespace std;

enum State{ RUNNING, BLOCKED, READY};

typedef struct Thread{

    unsigned int ID;
    State state;
    typedef struct itimerval quantumRunTime;

}Thread;

typedef struct Schedule{
    list<Thread> readyList;
    list<Thread> blockList;
    Thread runningThread;

    int quantumTime;
    priority_queue<int, vector<int>, greater<int> > ID_unused;
};

#endif //DEFINITIONS_H
