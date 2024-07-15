#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdbool.h>

// DO NOT INCLUDE
// ThreadPool is opeque
// #include "asyncio.h"

typedef void TaskArgVoid;

typedef struct {
    void (*handler)(TaskArgVoid*);
    TaskArgVoid* arg;
} Task;

typedef struct {
    unsigned int capacity;
    unsigned int count;
    Task* queue;
    unsigned int head; // pop position
    unsigned int tail; // push position
} TaskQueue;

TaskQueue task_queue_new_empty(unsigned int capacity) {
    return (TaskQueue) {
        .capacity = capacity,
        .count = 0,
        .queue = malloc(capacity * sizeof(Task)),
        .head = 0,
        .tail = 0,
    };
}

void task_queue_destroy(TaskQueue* tq) {
    tq->capacity = 0;
    tq->count = 0;
    free(tq->queue);
    tq->head= 0;
    tq->tail = 0;
}

bool task_queue_is_full(TaskQueue* tq) {
    return tq->count == tq->capacity;
}

bool task_queue_is_empty(TaskQueue* tq) {
    return tq->count == 0;
}

bool task_queue_has_next(TaskQueue* tq) {
    return tq->count > 0;
}

bool task_queue_push_task(TaskQueue* tq, Task task) {
    if (task_queue_is_full(tq)) {
        return false; // TODO: Handle Err
    }
    tq->queue[tq->tail] = task;
    tq->tail = (tq->tail + 1) % tq->capacity;
    tq->count++;
    return true;
}

Task task_queue_pop_task_unchecked(TaskQueue* tq) {
    Task task = tq->queue[tq->head];
    tq->count--;
    tq->head = (tq->head + 1) % tq->capacity;
    return task;
}

typedef enum {
    // NO_SHUTDOWN = 0
    THREADPOOL_IMMEDIATE_SHUTDOWN = 1,
    THREADPOOL_GRACEFULL_SHUTDOWN = 2,
} ThreadPoolShutdown;

typedef struct {
    unsigned int n_threads;
    // DWORD* thread_ids;
    unsigned int* thread_ids;
    HANDLE* threads;
    TaskQueue task_queue;
    CRITICAL_SECTION task_queue_lock;
    CONDITION_VARIABLE task_queue_notify;
    int shutdown;
    int running_threads;
} ThreadPool;

// DWORD WINAPI thread_pool_worker_thread(LPVOID lpParam);
unsigned int __stdcall thread_pool_worker_thread(void* thread_param);

ThreadPool* thread_pool_create(unsigned int n_threads, unsigned int task_queue_capacity) {
    unsigned int N_THREADS = n_threads;
    unsigned int TASK_QUEUE_CAPACITY = task_queue_capacity;

    ThreadPool* thread_pool = malloc(sizeof(ThreadPool));
    thread_pool->n_threads = N_THREADS;
    // thread_pool->thread_ids = malloc(N_THREADS * sizeof(DWORD));
    thread_pool->thread_ids = malloc(N_THREADS * sizeof(unsigned int));
    thread_pool->threads = malloc(N_THREADS * sizeof(HANDLE));
    thread_pool->task_queue = task_queue_new_empty(TASK_QUEUE_CAPACITY);
    InitializeCriticalSection(&thread_pool->task_queue_lock);
    InitializeConditionVariable(&thread_pool->task_queue_notify);
    thread_pool->shutdown = 0;
    thread_pool->running_threads = 0;

    for (int i = 0; i < N_THREADS; i++) {
        // thread_pool->threads[i] = CreateThread(
        //     NULL,                       // default security attributes
        //     0,                          // use default stack size  
        //     thread_pool_worker_thread,  // thread function name
        //     &thread_pool,               // argument to thread function 
        //     0,                          // use default creation flags 
        //     &thread_pool->thread_ids[i] // returns the thread identifier 
        // );
        thread_pool->threads[i] = (HANDLE) _beginthreadex( // NATIVE CODE
            NULL,                       // default security attributes
            0,                          // use default stack size  
            thread_pool_worker_thread,  // thread function name
            thread_pool,                // argument to thread function 
            0,                          // use default creation flags 
            &thread_pool->thread_ids[i] // returns the thread identifier 
        );

        // if (thread_pool->threads[i] == NULL) {
        //     DWORD err = GetLastError();
        //     printf("worker: %d, err: %d\n", i, err);
        // }
        
        thread_pool->running_threads++;
    }

    return thread_pool;
}

bool thread_pool_add_task(ThreadPool* pool, Task task) {
    bool add_success;
    
    EnterCriticalSection(&pool->task_queue_lock);

    if (pool->shutdown) {
        goto TASK_ADD_ERROR;
    }

    add_success = task_queue_push_task(&pool->task_queue, task);

    if (!add_success) {
        goto TASK_ADD_ERROR;
    }

    LeaveCriticalSection(&pool->task_queue_lock);
    WakeConditionVariable(&pool->task_queue_notify);

    return true;

    TASK_ADD_ERROR:
    LeaveCriticalSection(&pool->task_queue_lock);
    return false;
}

void thread_pool_free(ThreadPool* pool) {
    // if (pool == NULL || pool->running_threads > 0) {
    //     return;
    // }

    free(pool->thread_ids);
    free(pool->threads);
    task_queue_destroy(&pool->task_queue);
    DeleteCriticalSection(&pool->task_queue_lock);
    free(pool);
}

void thread_pool_shutdown(ThreadPool* pool, ThreadPoolShutdown shutdown) {

    EnterCriticalSection(&pool->task_queue_lock);

    if (pool->shutdown) {
        LeaveCriticalSection(&pool->task_queue_lock);
        return;
    }

    pool->shutdown = shutdown;

    LeaveCriticalSection(&pool->task_queue_lock);
    WakeAllConditionVariable(&pool->task_queue_notify);

    for (int i = 0; i < pool->n_threads; i++) {
        WaitForSingleObject(pool->threads[i], INFINITE);
    }
    for (int i = 0; i < pool->n_threads; i++) {
        CloseHandle(pool->threads[i]);
    }

    thread_pool_free(pool);
}

void example_async_task_print_int_arg(TaskArgVoid* arg);

// DWORD WINAPI thread_pool_worker_thread(LPVOID lpParam) {
//     ThreadPool* pool = lpParam;
unsigned int __stdcall thread_pool_worker_thread(void* thread_param) {
    ThreadPool* pool = thread_param;
    Task task;

    while (true) {
        EnterCriticalSection(&pool->task_queue_lock);

        while (task_queue_is_empty(&pool->task_queue) && !pool->shutdown) {
            SleepConditionVariableCS(&pool->task_queue_notify, &pool->task_queue_lock, INFINITE);
        }

        if (pool->shutdown == THREADPOOL_IMMEDIATE_SHUTDOWN
            || (pool->shutdown == THREADPOOL_GRACEFULL_SHUTDOWN
                && task_queue_is_empty(&pool->task_queue))
        ) {
            break;
        }

        if (task_queue_has_next(&pool->task_queue)) {
            task = task_queue_pop_task_unchecked(&pool->task_queue);
            LeaveCriticalSection(&pool->task_queue_lock);
            printf(" << Task Execution >> \n");
            task.handler(task.arg);
        }
        else {
            LeaveCriticalSection(&pool->task_queue_lock);
        }
    }

    pool->running_threads--;
    LeaveCriticalSection(&pool->task_queue_lock);
    // ExitThread(0);
    _endthreadex(0);
}

void example_async_task_print_int_arg(TaskArgVoid* arg) {
    int* arg_int = arg;
    printf("[Async] working for: %d\n", *arg_int);
    SleepEx(5000, false);
    printf("[Async] result: 2 * arg = %d\n", 2 * (*arg_int));
}
