
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define TIME_QUANTUM 3  // 3 ms time quantum
#define PROCESSES_NUM 5
#define MAX_BLOCKS 512  // 512 total blocks

// Process structure
typedef struct {
    int pid;
    int arrival_time;  // Changed from struct timeval to int
    int duration;
    int remaining_time;
    int memory_needed;
    bool in_memory;
} Process;

// Memory block structure
typedef struct {
    int start;
    int size;
    bool free;
    int pid;
} MemoryBlock;

// Queue node
typedef struct Node {
    Process *process;
    struct Node *next;
} Node;

// Queue structure
typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

// ------------------ Queue Functions ------------------

Queue* createQueue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    if (q == NULL) {
        perror("Failed to allocate memory for queue");
        exit(EXIT_FAILURE);
    }
    q->front = NULL;
    q->rear = NULL;
    return q;
}

int isQueueEmpty(Queue *q) {
    return (q->front == NULL);
}

void enqueue(Queue *q, Process* p) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    newNode->process = p;
    newNode->next = NULL;

    if (isQueueEmpty(q)) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

Process* dequeue(Queue *q) {
    if (isQueueEmpty(q)) {
        fprintf(stderr, "Error: Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    Node *temp = q->front;
    Process *p = temp->process;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return p;
}

void printQueue(Queue* queue) {
    if (isQueueEmpty(queue)) {
        printf("Queue is empty.\n");
        return;
    }

    Queue* tempQueue = (Queue*)malloc(sizeof(Queue));
    tempQueue->front = NULL;
    tempQueue->rear = NULL;

    while (!isQueueEmpty(queue)) {
        Process* p = dequeue(queue);
        printf("Process ID: %d, Arrival Time: %d, Duration: %d, Memory Needed: %d, Remaining Time: %d, In Memory: %s\n",
               p->pid, p->arrival_time, p->duration, p->memory_needed, p->remaining_time, p->in_memory ? "Yes" : "No");
        enqueue(tempQueue, p);
    }

    while (!isQueueEmpty(tempQueue)) {
        enqueue(queue, dequeue(tempQueue));
    }

    free(tempQueue);
}

// ------------------ Memory Management ------------------

MemoryBlock memory[MAX_BLOCKS];

void initialize_memory() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        memory[i].start = i;
        memory[i].size = 1;
        memory[i].free = true;
        memory[i].pid = -1;
    }
}

int first_fit(int memory_needed, int pid) {
    int free_blocks = 0, start_index = -1;

    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (memory[i].free) {
            if (start_index == -1) start_index = i;
            free_blocks++;
            if (free_blocks == memory_needed) {
                for (int j = start_index; j < start_index + memory_needed; j++) {
                    memory[j].free = false;
                    memory[j].pid = pid;
                }
                return 1;
            }
        } else {
            free_blocks = 0;
            start_index = -1;
        }
    }
    return -1;
}

void release_memory(MemoryBlock memory[], int pid) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (memory[i].pid == pid) {
            memory[i].free = true;
            memory[i].pid = -1;
        }
    }
}

void print_memory_state() {
    int i, end_index;
    int start_index = 0;
    int chunk = 0;

    for (i = 0; i < MAX_BLOCKS - 1; i++) {
        if (memory[i].pid == memory[i + 1].pid) {
            end_index = i + 1;
        } else {
            end_index = i;
            if (memory[i].free) {
                printf("Chunk of memory number %d: start at %d end at %d.\t State: Free\n", chunk, start_index, end_index);
            } else {
                printf("Chunk of memory number %d: start at %d end at %d.\t State: Used by process %d\n", chunk, start_index, end_index, memory[i].pid);
            }
            chunk++;
            start_index = i + 1;
        }
    }

    if (memory[MAX_BLOCKS - 1].free) {
        printf("Chunk of memory number %d: start at %d end at %d.\t State: Free\n", chunk, start_index, MAX_BLOCKS - 1);
    } else {
        printf("Chunk of memory number %d: start at %d end at %d.\t State: Used by process %d\n", chunk, start_index, MAX_BLOCKS - 1, memory[MAX_BLOCKS - 1].pid);
    }
}

// ------------------ MergeSort for Queue ------------------

Node* split(Node* head) {
    Node* fast = head->next;
    Node* slow = head;

    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }

    Node* middle = slow->next;
    slow->next = NULL;
    return middle;
}

Node* merge(Node* left, Node* right) {
    if (!left) return right;
    if (!right) return left;

    if (left->process->arrival_time < right->process->arrival_time) {
        left->next = merge(left->next, right);
        return left;
    } else {
        right->next = merge(left, right->next);
        return right;
    }
}

Node* mergeSort(Node* head) {
    if (!head || !head->next) {
        return head;
    }

    Node* middle = split(head);

    Node* left = mergeSort(head);
    Node* right = mergeSort(middle);

    return merge(left, right);
}

Queue* sortQueueByArrivalTime(Queue* originalQueue) {
    if (isQueueEmpty(originalQueue)) {
        return originalQueue;
    }

    originalQueue->front = mergeSort(originalQueue->front);

    Node* temp = originalQueue->front;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    originalQueue->rear = temp;

    return originalQueue;
}

// ------------------ Reading Processes ------------------

void readProcesses(Queue* processes) {
    for (int i = 0; i < PROCESSES_NUM; i++) {
        Process *p = (Process*)malloc(sizeof(Process));
        if (!p) {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }
        printf("Enter details for Process %d (arrival_time [int], duration, memory_needed): ", i);
        scanf("%d %d %d", &p->arrival_time, &p->duration, &p->memory_needed);
        p->pid = i;
        p->remaining_time = p->duration;
        p->in_memory = false;
        enqueue(processes, p);
    }
}

//-----------------------------------PRINTING----------------------------------------------
void print_process_stats(Process* p) {
    struct timeval current_time;
    printf("Process %d has finished \n", p->pid);
}




//-----------------------------------RR---------------------------------------------

void roundRobin (Queue* processes){
    int time =1;
    while (!isQueueEmpty(processes))
    {
        Process* current = dequeue(processes);
        if (current->arrival_time > time)
        {
            printf("...\n");
            
            enqueue(processes, current); 
        }else {
        //if the process is not in memory, we try to load it
        if (current->in_memory == false)
        {
            if (first_fit(current->memory_needed, current->pid) != -1)
            {
                current->in_memory = true;
                printf("Process %d loaded into memory.\n",current->pid);
                print_memory_state();
            } else {
                printf(" Process %d cannot be loaded into memory.\n",  current->pid);
                enqueue(processes, current);
            }
        }

        if (current->in_memory==true) {
            printf("Simulating process %d\n", current->pid);
            current->remaining_time -= TIME_QUANTUM;
            time += TIME_QUANTUM;
            if (current->remaining_time <= 0)
            {
                print_process_stats(current);
                release_memory(memory, current->pid);
            } else {
                printf("Process %d has %d s remaining\n", current->pid, current->remaining_time);
                enqueue(processes, current);
            }
        }

        }
        time++;
        

    }
    
}



int main() {
    initialize_memory();
    Queue *processes = createQueue();
    readProcesses(processes);
    processes = sortQueueByArrivalTime(processes);
    printQueue(processes);
    roundRobin(processes);
    return 0;
}
