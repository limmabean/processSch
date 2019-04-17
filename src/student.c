/*
 * student.c
 * Multithreaded OS Simulation for ECE 3056
 * Spring 2019
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Matthew Lim
 * GTID: 903211953
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"
/* Define which scheduler we are using.
 * FCFS = 0
 * Priority Queue = 1
 * SJF = 2
 */

#define FCFS 0
#define PRIORITYQ 1
#define SJF 2

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
static pcb_t *FCFSqueue;
static pthread_mutex_t running_processes_mutex;

static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;
unsigned int cpu_count;
unsigned int scheduler_type;



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
    pthread_mutex_lock(&running_processes_mutex);
    pcb_t *readyQIterator = readyQHead;
    pcb_t *selectedProcess = readyQHead;

    // lets test the ready queue before schedule
    pcb_t * prev = NULL;
    pcb_t *iter = readyQHead;
    printf("Schedule!!\n");
    while(iter!=NULL){
        printf("Name: %s, PID: %d, Priority: %d\n", iter->name, iter->pid, iter->priority);
        iter = iter->next;
    }

    /* ============================================================FCFS============================================= */
    if(scheduler_type == FCFS) {
        if (readyQIterator != NULL) {
            unsigned int min_PID = readyQIterator->pid;
            // This finds the min_PID in the queue
            while(readyQIterator != NULL){
                if(readyQIterator->pid < min_PID){ min_PID = readyQIterator->pid; }
                readyQIterator= readyQIterator->next;
            }
            // This finds the node before min_PID in the queue
            readyQIterator = readyQHead;
            while(readyQIterator->next != NULL) {
                if((readyQIterator->next)->pid == min_PID) {
                    break;
                }
                readyQIterator = readyQIterator->next;
            }
            //If there is only one value in readyQ
            if(readyQHead->next == NULL){
                selectedProcess->state = PROCESS_RUNNING;
                running_processes[cpu_id] = selectedProcess;
                readyQHead = NULL;
            } else {
                // If the selectedProcess is the head but there is another value
                if(readyQHead->pid == min_PID){
                    selectedProcess = readyQHead;
                    readyQHead = readyQHead->next;
                    selectedProcess->next = NULL;
                    selectedProcess->state = PROCESS_RUNNING;
                    running_processes[cpu_id] = selectedProcess;
                } else {
                    selectedProcess = (readyQIterator->next);
                    // Set node before min_PID's next to selectedProcesses's next
                    readyQIterator->next = selectedProcess->next;
                    selectedProcess->next = NULL;
                    selectedProcess->state = PROCESS_RUNNING;
                    running_processes[cpu_id] = selectedProcess;
                }
            }
        }
    }
    /* ======================================================PRIORITY=============================================== */
    else if (scheduler_type == PRIORITYQ){
        if (readyQIterator != NULL) {
            int max_Priority = readyQIterator->priority;
            // This finds the max_Priority in the queue
            while(readyQIterator != NULL){
                if(readyQIterator->priority > max_Priority){ max_Priority = readyQIterator->priority; }
                readyQIterator= readyQIterator->next;
            }
            // This finds the node before max_Priority in the queue
            readyQIterator = readyQHead;
            while(readyQIterator->next != NULL) {
                if((readyQIterator->next)->priority == max_Priority) {
                    break;
                }
                readyQIterator = readyQIterator->next;
            }
            //If there is only one value in readyQ
            if(readyQHead->next == NULL){
                selectedProcess->state = PROCESS_RUNNING;
                running_processes[cpu_id] = selectedProcess;
                readyQHead = NULL;
            } else {
                // If the selectedProcess is the head but there is another value
                if(readyQHead->priority == max_Priority){
                    selectedProcess = readyQHead;
                    readyQHead = readyQHead->next;
                    selectedProcess->next = NULL;
                    selectedProcess->state = PROCESS_RUNNING;
                    running_processes[cpu_id] = selectedProcess;
                } else {
                    selectedProcess = (readyQIterator->next);
                    // Set node before min_PID's next to selectedProcesses's next
                    readyQIterator->next = selectedProcess->next;
                    selectedProcess->next = NULL;
                    selectedProcess->state = PROCESS_RUNNING;
                    running_processes[cpu_id] = selectedProcess;
                }
            }
        }
    }
    /* =======================SJF====================== */
    else if (scheduler_type == SJF){


    } else {
        printf("Incorrect type.");
    }
    context_switch(cpu_id, selectedProcess);
    pthread_mutex_unlock(&running_processes_mutex);
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
    // Lock the queue & running processes
    // Take process out of running_processes
    pcb_t *preemptedProcess = running_processes[cpu_id];
    running_processes[cpu_id] = NULL;
    // Mark the process as ready
    preemptedProcess->state = PROCESS_READY;
    // If the readyQ is empty preempt should trigger queue_not_empty
    if (readyQHead == NULL) {
        readyQHead = preemptedProcess;
        pthread_cond_signal(&queue_not_empty);
    } else {
        // Linked List that adds to the beginning
        // Insert into ready queue [new process] -> [old process]
        preemptedProcess->next = readyQHead;
        readyQHead = preemptedProcess;
    }

    //Lets test ready queue after preempt
    pcb_t *iter = readyQHead;
    printf("Preempt!!\n");
    while(iter!=NULL){ printf("Name: %s, PID: %d, Priority: %d\n", iter->name, iter->pid, iter->priority);iter = iter->next;}

    // Unlock the queue & running processes
    pthread_mutex_unlock(&running_processes_mutex);
    pthread_mutex_unlock(&queue_mutex);
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
    // Same comments as terminate
    pthread_mutex_lock(&queue_mutex);
    pthread_mutex_lock(&running_processes_mutex);
    pcb_t *currentProcess = running_processes[cpu_id];
    currentProcess->state = PROCESS_WAITING;
    running_processes[cpu_id] = NULL;
    pthread_mutex_unlock(&running_processes_mutex);
    pthread_mutex_unlock(&queue_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    // Lock the running processes
    pthread_mutex_lock(&queue_mutex);
    pthread_mutex_lock(&running_processes_mutex);
    // Get the process currently running
	pcb_t *currentProcess = running_processes[cpu_id];
	// Set the state as finished
	currentProcess->state = PROCESS_TERMINATED;
	running_processes[cpu_id] = NULL;
	// Unlock the running processes
	printf("TERMINATED");
	printf(currentProcess->name);
    pthread_mutex_unlock(&running_processes_mutex);
    pthread_mutex_unlock(&queue_mutex);
    // Call schedule() to select new process
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
    pthread_mutex_lock(&queue_mutex);
    pthread_mutex_lock(&running_processes_mutex);
    // IF SOMETHING IN QUEUE
    if(readyQHead != NULL) {
        // Linked List that adds to the beginning
        // Mark the process as ready
        process->state = PROCESS_READY;
        // Insert into ready queue [new process] -> [old process]
        process->next = readyQHead;
        readyQHead = process;
    } else {
        // Mark the process as ready
        process->state = PROCESS_READY;
        // Insert into ready queue readyQHead -> [new process]
        readyQHead = process;
        pthread_cond_signal(&queue_not_empty);
    }

    if(scheduler_type == PRIORITYQ) {
        int noFreeCPU = 1;
        int lowestPriority = process->priority;
        int lowestPriorityCPU = -1;
        for (int i = 0; i < cpu_count; i++) {
            // If any of the running processes are NULL then there is a Free CPU
            if (running_processes[i] == NULL) {
                noFreeCPU = 0;
            } else {
                // Run through the running processes to find the lowest priority
                if ((running_processes[i])->priority < lowestPriority) {
                    lowestPriority = (running_processes[i])->priority;
                    lowestPriorityCPU = i;
                }
            }
        }
        // If there are no free CPU's and the lowest priority is less than the new process's then preempt
        if (noFreeCPU == 1 && lowestPriority < process->priority) {
            force_preempt(lowestPriorityCPU);
        }
    }

    //Lets test ready queue after wake up
    pcb_t* iter = readyQHead;
    printf("Wakeup!!\n");
    while(iter!=NULL){printf("Name: %s, PID: %d, Priority: %d\n", iter->name, iter->pid, iter->priority);iter = iter->next;}

    pthread_mutex_unlock(&running_processes_mutex);
    pthread_mutex_unlock(&queue_mutex);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -s command-line parameters.
 */
int main(int argc, char *argv[])
{
    //Parse scheduler type
    if(strcmp(argv[1],"-f") == 0) {
        scheduler_type = FCFS;
    } else if (strcmp(argv[1],"-p") == 0) {
        scheduler_type = PRIORITYQ;
    } else if (strcmp(argv[1],"-s") == 0) {
        scheduler_type = SJF;
    } else {
        printf("Must input a correct scheduler type!");
    }
    //Parse cpu_count (sim has handler)
    cpu_count = atoi(argv[2]);
    // Allocate the running_processes[] array and its mutex */
    running_processes = malloc(sizeof(pcb_t*) * cpu_count);
    for(int i = 0; i < cpu_count; i++) {
        running_processes[i] = NULL;
    }
    // Define the Head of Ready Queue
    readyQHead = malloc(sizeof(pcb_t*));
    readyQHead = NULL;
    assert(running_processes != NULL);
    pthread_mutex_init(&running_processes_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);
    free(readyQHead);
    return 0;
}

