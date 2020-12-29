/*
Robert Iakobashvili, MIT, Apache or BSD license - whatever you wish.

Compile as: gcc threads.c -o threads -lpthread
Objdump it as: objdump -d threads

Run it as ./threads
*/
#define _GNU_SOURCE

#include <stdio.h>     
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#define CYCLESNUM 10

void* thrFunc(void *arg);



int main()
{
    pthread_t firstThrId;
    pthread_t secondThrId;

    int rval = pthread_create(&firstThrId, NULL, thrFunc, 0);

    rval = pthread_create(&secondThrId, NULL, thrFunc, 0);


    //rval = pthread_join(firstThrId, NULL);
    //rval = pthread_join(secondThrId, NULL);

    //sleep(2);
    pthread_exit(0);
    return 0;
}

void* thrFunc(void *arg)
{
    for (int k = 0; k < CYCLESNUM; k++)
    {
        sleep(1);
        //printf ("%d\n", k);
        fprintf (stderr, "%d\n", k);
    }
    return NULL;
}
