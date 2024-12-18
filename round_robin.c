#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_QUEUE_SIZE 10
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
    int *data; // Pointer to the dynamic array
    int front;
    int rear;
    int capacity;
} DynamicQueue;

// Initialize the dynamic queue
void init_queue(DynamicQueue *q) {
    q->capacity = INITIAL_QUEUE_SIZE;
    q->data = (int *) malloc(q->capacity * sizeof(int));
    q->front = 0;
    q->rear = 0;
    memset(q->data, -1, q->capacity * sizeof(int)); // Initialize all elements to -1
}

// Enqueue an element
void enqueue(DynamicQueue *q, int value) {
    // Resize the queue if it's full
    if ((q->rear + 1) % q->capacity == q->front) {
        int old_capacity = q->capacity;
        q->capacity *= 2; // Double the capacity
        q->data = (int *) realloc(q->data, q->capacity * sizeof(int));
        if (q->data == NULL) {
            printf("Memory reallocation failed!\n");
            exit(EXIT_FAILURE);
        }
        // Initialize the newly allocated memory to -1
        memset(q->data + old_capacity, -1, (q->capacity - old_capacity) * sizeof(int));
        printf("Queue resized from %d to %d\n", old_capacity, q->capacity);
    }
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
}

// Dequeue an element
int dequeue(DynamicQueue *q) {
    if (q->front == q->rear) {
        printf("Queue is empty!\n");
        return -1; // Error code
    }
    int value = q->data[q->front];
    q->data[q->front] = -1; // Reset the dequeued position to -1
    q->front = (q->front + 1) % q->capacity;
    return value;
}

// Check if the queue is empty
bool is_empty(DynamicQueue *q) {
    return q->front == q->rear;
}

// Free the memory used by the queue
void free_queue(DynamicQueue *q) {
    free(q->data);
}

int peek(DynamicQueue *q) {
    if (q->front == q->rear) {
        printf("Queue is empty!\n");
        return -1; // Error code for an empty queue
    }
    return q->data[q->front];
}

// Calculate metrics
void calculate_metrics(Process processes[], int n, int total_time, int cpu_busy_time) {
    float avg_turnaround = 0, avg_waiting = 0, avg_response = 0;
    float cpu_utilization = (float) cpu_busy_time / total_time * 100;

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

void round_robin(Process processes[], int process_count, int time_quantum, int io_wait_time) {
    int time = 0, completed = 0, cpu_busy_time = 0;
    int io_timer[MAX_PROCESSES] = {0}; // I/O timers for processes

    DynamicQueue queue;
    init_queue(&queue);

    printf("\nResult:\nTime  Process ID  Status     Initial Remaining Time  Remaining Time\n");

    // Sort processes by arrival_time before enqueueing
    for (int i = 0; i < process_count - 1; i++) {
        for (int j = 0; j < process_count - i - 1; j++) {
            if (processes[j].arrival_time > processes[j + 1].arrival_time) {
                Process temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }

    // Enqueue all processes based on arrival time
    for (int i = 0; i < process_count; i++) {
        enqueue(&queue, i);
    }

    while (completed < process_count) {
        // Add newly arrived processes
        // for (int i = 0; i < process_count; i++) {
        //     if (processes[i].arrival_time <= time && !processes[i].is_completed && !in_ready_queue[i]) {
        //         enqueue(&queue, i);
        //         in_ready_queue[i] = true;
        //     }
        // }

        // Handle blocked processes completing I/O
        for (int i = 0; i < process_count; i++) {
            if (io_timer[i] > 0) {
                io_timer[i]--;
                if (io_timer[i] == 0 && !processes[i].is_completed) {
                    enqueue(&queue, i);
                }
            }
        }

        // Execute the next process
        if (!is_empty(&queue)) {
            int current = peek(&queue);

            // Check if the process's arrival time matches the current time or fits logic
            if (processes[current].arrival_time <= time) {
                dequeue(&queue);
                if (processes[current].is_first_run) {
                    processes[current].response_time = time - processes[current].arrival_time;
                    processes[current].is_first_run = false;
                }

                int initial_remaining_time = processes[current].remaining_time;

                int run_time = (processes[current].remaining_time > time_quantum)
                                   ? time_quantum
                                   : processes[current].remaining_time;
                processes[current].remaining_time -= run_time;
                cpu_busy_time += run_time;
                time += run_time;

                printf("%4d %11d  Running %26d %17d\n", time - run_time, processes[current].id,
                       initial_remaining_time, processes[current].remaining_time);

                if (processes[current].remaining_time == 0) {
                    processes[current].is_completed = true;
                    completed++;
                    processes[current].turnaround_time = time - processes[current].arrival_time;
                    processes[current].waiting_time =
                            processes[current].turnaround_time - processes[current].burst_time;
                } else {
                    io_timer[current] = io_wait_time;
                    printf("%4d %11d  Blocked %26d %17d\n", time, processes[current].id,
                           initial_remaining_time, processes[current].remaining_time);
                }
            } else {
                time++;
            }
        }
    }

    calculate_metrics(processes, process_count, time, cpu_busy_time);
    free_queue(&queue);
}

int main() {
    char input[64];

    int process_count, time_quantum, io_wait_time;
    Process processes[MAX_PROCESSES];

    while (1) {
        printf("Enter number of processes (1-10): ");
        fgets(input, sizeof(input), stdin);
        process_count = strtol(input, NULL, 10);
        if (process_count > 10 || process_count < 1) {
            printf("Invalid number. Please enter a number between 1 and 10\n");
            continue;
        }
        break;
    }

    while (1) {
        printf("Enter time quantum (TQ): ");
        fgets(input, sizeof(input), stdin);
        time_quantum = strtol(input, NULL, 10);
        if (time_quantum <= 0) {
            printf("Invalid number. Please enter a number greater than zero\n");
            continue;
        }
        break;
    }

    while (1) {
        printf("Enter I/O wait time: ");
        fgets(input, sizeof(input), stdin);
        io_wait_time = strtol(input, NULL, 10);
        if (io_wait_time < 0) {
            printf("Invalid number. Please enter a number greater than or equal to zero\n");
            continue;
        }
        break;
    }

    printf("Enter arrival times, burst times for your processes:\n");
    for (int i = 0; i < process_count; i++) {
        printf("Process %d:\n", i + 1);
        processes[i].id = i + 1;
        while (1) {
            printf("  Arrival Time = ");
            fgets(input, sizeof(input), stdin);
            const int curr_arrival_time = strtol(input, NULL, 10);
            if (curr_arrival_time < 0) {
                printf("Invalid number. Please enter a number greater than or equal to zero\n");
                continue;
            }
            processes[i].arrival_time = curr_arrival_time;
            break;
        }

        while (1) {
            printf("  Burst Time = ");
            fgets(input, sizeof(input), stdin);
            const int burst_time = strtol(input, NULL, 10);
            if (burst_time <= 0) {
                printf("Invalid number. Please enter a number greater than or equal to zero\n");
                continue;
            }
            processes[i].burst_time = burst_time;
            break;
        }
        processes[i].io_wait_time = io_wait_time;
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].is_completed = false;
        processes[i].is_first_run = true;
    }

    round_robin(processes, process_count, time_quantum, io_wait_time);

    return 0;
}
