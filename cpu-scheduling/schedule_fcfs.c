#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"

struct node *g_head = NULL;                         // head of list

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->name = strdup(name);
    free(name);
    insert(&g_head, newTask);                       // &g_head = NULL, newTask = new task from file
}

bool comesBefore(char *a, char *b) { return strcmp(a, b) < 0; }

// based on traverse from list.c
// finds the task whose name comes first in dictionary
Task *pickNextTask() {
    // if list is empty, nothing to do
    if (!g_head)
        return NULL;

    struct node *temp;
    temp = g_head;
    Task *best_sofar = temp->task;

    while (temp != NULL) {
        if (comesBefore(temp->task->name, best_sofar->name)) {
            best_sofar = temp->task;
        }
        temp = temp->next;
    }

    return best_sofar;
}

void schedule() {
    int current_time = 0;
    Task* task = pickNextTask();
    while(task) {
        run(task, task->burst);
        current_time += task->burst;
        delete(&g_head, task);
        task = pickNextTask();
        printf("Time is now: %d\n", current_time);
    }
}

