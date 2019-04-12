/*
 * student.c
 * Multithreaded OS Simulation for ECE 3056
 * Spring 2019
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: 
 * GTID:
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


/*
 * running_processes[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * running_processes[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the running_processes[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  running_processes_mutex has been provided
 * for your use.
 */
static pcb_t *readyQHead;
static pcb_t **running_processes;
static pthread_mutex_t running_processes_mutex;

static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;
unsigned int cpu_count;


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which
 *  you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *  The running_processes array (see above) is how you access the currently running process indexed by the cpu id.
 *  See above for full description.
 *  context_switch() is prototyped in os-sim.h. Look there for more information
 *  about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
	pthread_mutex_lock(&queue_mutex);
	if(queue_not_empty) {
		pcb_t *readyQIterator = readyQHead;
		//If the readyQ only has one value (This is to prevent seg faults)
        if(readyQIterator->next = NULL) {
            //Set state to running
            // [ RUNNING ] -> NULL
            readyQIterator->state = PROCESS_RUNNING;
            //Set readyQHead = NULL;
            // NULL
            readyQIterator = NULL;
        } else {
            //Iterate to second to the end;
            // [ X ] -> [   ] -> NULL
            while((readyQIterator->next)->next != NULL) {
                readyQIterator = readyQIterator->next;
            }
            //Set the end's state to running
            // [ X ] -> [running] -> NULL
            (readyQIterator->next)->state = PROCESS_RUNNING;
            //Remove the process from the end of the list by
            // [ X ] -> NULL
            readyQIterator->next = NULL;
        }
        context_switch(cpu_id, readyQIterator);
	} else {
		context_switch(cpu_id, NULL);
	}
	pthread_mutex_unlock(&queue_mutex);
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&queue_mutex);
    while(readyQHead == NULL){
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted 
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    //Lock the queue
    pthread_mutex_lock(&queue_mutex);
    pcb_t *readyQIterator = readyQHead;
    if (readyQHead == NULL) {

    } else {
        //Iterate to the end;
        while(readyQIterator->next != NULL) {
            readyQIterator = readyQIterator->next;
        }
        //Lock the running processes
        pthread_mutex_lock(&running_processes_mutex);
        //Place the running process into the readyQ
        readyQIterator->next = running_processes[cpu_id];
        //Set it's state to ready
        (readyQIterator->next)->state = PROCESS_READY;
        //Unlock the running processes
        pthread_mutex_unlock(&running_processes_mutex);
        //Unlock the queue
        pthread_mutex_unlock(&queue_mutex);
    }
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    //Lock the running processes
    pthread_mutex_lock(&running_processes_mutex);
    //Get the process currently running
    pcb_t *currentProcess = running_processes[cpu_id];
    //Set the state as waiting
    currentProcess->state = PROCESS_WAITING;
    //Unlock the running processes
    pthread_mutex_unlock(&running_processes_mutex);
    //Call schedule() to select new process
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    //Lock the running processes
    pthread_mutex_lock(&running_processes_mutex);
    //Get the process currently running
	pcb_t *currentProcess = running_processes[cpu_id];
	//Set the state as finished
	currentProcess->state = PROCESS_TERMINATED;
	//Unlock the running processes
    pthread_mutex_unlock(&running_processes_mutex);
    //Call schedule() to select new process
	schedule(cpu_id);
}

/*
 * wake_up() is the handler called by t he simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *  To preempt a process, use force_preempt(). Look in os-sim.h for
 *  its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    //IF EMPTY/NOTHING IN QUEUE
    if(readyQHead == NULL) {
        //FIFO
        //Mark the process as ready
        process->state = PROCESS_READY;
        //Insert into ready queue [new process] -> [old process]
        process->next = readyQHead;
        readyQHead = process;
        pthread_cond_signal(&queue_not_empty);
    } else {
        //FIFO
        //Mark the process as ready
        process->state = PROCESS_READY;
        //Insert into ready queue [new process] -> [old process]
        process->next = readyQHead;
        readyQHead = process;
    }

}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -s command-line parameters.
 */
int main(int argc, char *argv[])
{

    /* Uncomment this block once you have parsed the command line arguments
        to start scheduling yout processes
    // Allocate the running_processes[] array and its mutex */
    running_processes = malloc(sizeof(pcb_t*) * cpu_count);
    //Define the Head of Ready Queue
    readyQHead = malloc(sizeof(pcb_t*));

    assert(running_processes != NULL);
    pthread_mutex_init(&running_processes_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    pthread_mutex_destroy(&running_processes_mutex);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_destroy(&queue_not_empty);
    return 0;
}

