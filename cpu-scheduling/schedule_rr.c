#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"

int size = 0;
struct node *g_head = NULL;                         // head of list


void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->name = strdup(name);
    free(name);
    insert(&g_head, newTask);                       // &g_head = NULL, newTask = new task from file
    size++;
}


// based on traverse from list.c
// finds the next task in the loop for rounb robin
Task *pickNextTask(int currentNode) {
    // if list is empty, nothing to do
    if (!g_head) return NULL;

    struct node *temp;
    temp = g_head;
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

void schedule() {
    int current_time = 0;
    int currentNode = size-1;                   /* current node in the round robin (1-2-3-4..)*/
    int outOfQuantum = 0;                       /* bool check when a task has no burst left */
    int quantum = 10;

    Task* task = pickNextTask(currentNode);
    
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
            delete(&g_head, task);
            outOfQuantum= 0;
            size--;
        }

        task = pickNextTask(--currentNode);
        if(!currentNode) currentNode = size;

        printf("Time is now: %d\n", current_time);
    }
}
