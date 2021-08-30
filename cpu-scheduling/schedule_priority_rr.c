#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"

struct node *g_head = NULL;                         // head of list
struct node *c_head = NULL;                         // current head; new list for processes of equal priority
int current_time = 0;

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->name = strdup(name);
    free(name);
    insert(&g_head, newTask);                       // &g_head = NULL, newTask = new task from file
}

// based on traverse from list.c
// finds the next task in the loop for rounb robin
Task *pickNextTask_RR(int currentNode, int size) {
    // if list is empty, nothing to do
    if (!c_head) return NULL;

    struct node *temp = c_head;
    Task *next_task = temp->task;
    int i = 0;

    while (temp != NULL && i < size) {
        if (i == currentNode) {
            next_task = temp->task;
            break;
        }
        temp = temp->next;
        i++;
    }

    return next_task;
}

/* if two jobs have equal priority, round robin scheduling */
void schedule_rr(int size) {
    int currentNode = 0;                   
    int outOfQuantum = 0;
    int quantum = 10;

    Task* task = pickNextTask_RR(currentNode, size);

    while(task) {
        int time_ran = quantum;
        if((task->burst - quantum) <= 0) {      // check if burst does not use full quantm
            time_ran = task->burst;
            task->burst = 0;
            outOfQuantum = 1;
        }
        else {                                  // else, use full quantum
            task->burst -= quantum;
        }

        run(task, time_ran);
        current_time += time_ran;
        
        if(outOfQuantum) {
            delete(&c_head, task);
            outOfQuantum= 0;
            size--;
            currentNode--;
        }

        task = pickNextTask_RR(++currentNode, size);
        if(currentNode == size-1) currentNode = -1;

        printf("Time is now: %d\n", current_time);
    }
}

bool higherPriority(int a, int b) { return a > b; }

/* single process with highest priority, use full burst (depending on quantum) */
void schedule_priority() {
    int quantum = 10;
    int outOfQuantum = 0;
    Task* task = c_head->task;

    while(!outOfQuantum) {
        int time_ran = quantum;
        if((task->burst - quantum) <= 0) {      // check if burst does not use full quantm
            time_ran = task->burst;
            task->burst = 0;
            outOfQuantum = 1;
        }
        else {                                  // else, use full quantum
            task->burst -= quantum;
        }

        run(task, time_ran);
        current_time += time_ran;

        if(outOfQuantum) delete(&c_head, task);
        printf("Time is now: %d\n", current_time);
    }
}

// finds the next task in the loop for rounb robin
int pickAlgorithm() {   
    if(g_head == NULL) {return 0;}               
    
    struct node *temp = g_head;
    Task *best_sofar = temp->task;
    int h_priority = 0;
    int samePriority = 0;

    while (temp != NULL) {
        if (higherPriority(temp->task->priority, best_sofar->priority)) {       // find highest priority task
            best_sofar = temp->task;
            h_priority = best_sofar->priority;
            samePriority = 1;
        }
        else if(temp->task->priority == best_sofar->priority) {                 // tasks with same (highest) priority
            h_priority = best_sofar->priority;
            samePriority++;
        }
        temp = temp->next;
    }
    
    // insert all highest priority nodes into own (new) list
    struct node *next = NULL;
    temp = g_head;
    while(temp != NULL) {
        if(temp->task->priority == h_priority) {
            Task *newTask = malloc(sizeof(Task));
            newTask->priority = temp->task->priority;
            newTask->burst = temp->task->burst;
            newTask->name = strdup(temp->task->name);
            insert(&c_head, newTask);
            next = temp->next;
            delete(&g_head, temp->task);
            temp = next;
        }
        else {
            temp = temp->next;
        }
    }


    if(samePriority > 1) {
        schedule_rr(samePriority);
    }
    else {
        schedule_priority();
    }
    return 1;
}

// schedule either priority or round robin algorithm depending on if there are multiple processes with same priority
void schedule() {
    int test = 1;
    while(test) {
        test = pickAlgorithm();
    }
}

