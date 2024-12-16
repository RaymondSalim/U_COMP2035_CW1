#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 10

typedef struct {
    int id;
    int arrival_time;
    int burst_time;
    int io_wait_time;
    int remaining_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    bool is_completed;
    bool is_first_run;
} Process;

typedef struct {
    int time;
    int process_id;
} Table;

void calculate_metrics(Process processes[], int n, int total_time, int cpu_busy_time) {
    float avg_turnaround = 0, avg_waiting = 0, avg_response = 0;
    float cpu_utilization = (float)cpu_busy_time / total_time * 100;

    printf("\nRR Performance Result:\n");
    for (int i = 0; i < n; i++) {
        avg_turnaround += processes[i].turnaround_time;
        avg_waiting += processes[i].waiting_time;
        avg_response += processes[i].response_time;
        printf("Process %d: Turnaround Time = %d, Waiting Time = %d, Response Time = %d\n",
               processes[i].id, processes[i].turnaround_time, processes[i].waiting_time, processes[i].response_time);
    }
    printf("\nTotal CPU Utilization: %.2f%%\n", cpu_utilization);
    printf("Average Turnaround Time: %.2f\n", avg_turnaround / n);
    printf("Average Waiting Time: %.2f\n", avg_waiting / n);
    printf("Average Response Time: %.2f\n", avg_response / n);
}

void round_robin(Process processes[], int n, int time_quantum, int io_wait_time) {
    int time = 0;
    int completed = 0;
    int cpu_busy_time = 0;
    int front = 0, rear = 0;
    int ready_queue[MAX_PROCESSES];
    Table gantt[1000];
    int tbl_index = 0;

    memset(ready_queue, -1, sizeof(ready_queue));

    printf("\nResult:\nTime  Process ID  Status  Remaining Time\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].is_completed) {
                ready_queue[rear++] = i;
            }
        }

        if (front != rear) {
            int current = ready_queue[front++];

            if (processes[current].is_first_run) {
                processes[current].response_time = time - processes[current].arrival_time;
                processes[current].is_first_run = false;
            }

            int run_time = (processes[current].remaining_time > time_quantum) ? time_quantum : processes[current].remaining_time;
            processes[current].remaining_time -= run_time;
            cpu_busy_time += run_time;
            time += run_time;
            gantt[tbl_index++] = (Table){.time = time, .process_id = processes[current].id};

            printf("%4d %11d  Running   %d\n", time - run_time, processes[current].id, processes[current].remaining_time);

            if (processes[current].remaining_time == 0) {
                processes[current].is_completed = true;
                completed++;
                processes[current].turnaround_time = time - processes[current].arrival_time;
                processes[current].waiting_time = processes[current].turnaround_time - processes[current].burst_time;
            } else {
                ready_queue[rear++] = current; // Re-queue process
                time += io_wait_time;
            }
        } else { // No process ready
            time++;
        }
    }

    calculate_metrics(processes, n, time, cpu_busy_time);
}

int main() {
    int n, time_quantum, io_wait_time;
    Process processes[MAX_PROCESSES];

    printf("Enter number of processes (1-10): ");
    scanf("%d", &n);

    printf("Enter time quantum (TQ): ");
    scanf("%d", &time_quantum);

    printf("Enter I/O wait time: ");
    scanf("%d", &io_wait_time);

    printf("Enter arrival times, burst times for your processes:\n");
    for (int i = 0; i < n; i++) {
        printf("Process %d:\n", i + 1);
        processes[i].id = i + 1;
        printf("  Arrival Time = ");
        scanf("%d", &processes[i].arrival_time);
        printf("  Burst Time = ");
        scanf("%d", &processes[i].burst_time);
        processes[i].io_wait_time = io_wait_time;
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].is_completed = false;
        processes[i].is_first_run = true;
    }

    round_robin(processes, n, time_quantum, io_wait_time);

    return 0;
}
