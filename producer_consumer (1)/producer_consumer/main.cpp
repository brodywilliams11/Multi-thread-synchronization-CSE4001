// cse4001_sync.cpp
// Implements four synchronization problems from Downey's "Little Book of Semaphores"
//
// Compile: g++ cse4001_sync.cpp -lpthread -o cse4001_sync
// or run: make
//

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <iostream>
#include <time.h>

using namespace std;

/* Simple Semaphore wrapper around POSIX sem_t */
class Semaphore {
public:
    Semaphore(int initialValue = 0) {
        sem_init(&mSemaphore, 0, initialValue);
    }
    ~Semaphore() {
        sem_destroy(&mSemaphore);
    }
    void wait() {
        sem_wait(&mSemaphore);
    }
    void signal() {
        sem_post(&mSemaphore);
    }
private:
    sem_t mSemaphore;
};

// ---------- Global constants ----------
const int NUM_READERS = 5;
const int NUM_WRITERS = 5;
const int NUM_PHILOSOPHERS = 5;

// ---------- Problem 1 (No-starve readers-writers) globals ----------
Semaphore *p1_mutex = nullptr;
Semaphore *p1_roomEmpty = nullptr;
Semaphore *p1_turnstile = nullptr;
int p1_readers = 0;

// ---------- Problem 2 (Writer-priority readers-writers) globals ----------
Semaphore *p2_mutex = nullptr;
Semaphore *p2_roomEmpty = nullptr;
Semaphore *p2_turnstile = nullptr;
int p2_readers = 0;

// ---------- Problem 3 (Dining philosophers #1) globals ----------
Semaphore *p3_chopstick = nullptr;

// ---------- Problem 4 (Dining philosophers #2) globals ----------
Semaphore *p4_chopstick = nullptr;

// Utility: random sleep between min and max seconds (fractional)
void rand_sleep(double min_s, double max_s) {
    double r = (double)rand() / RAND_MAX;
    double t = min_s + r * (max_s - min_s);
    // convert to microseconds
    usleep((useconds_t)(t * 1e6));
}

// ---------------- Problem 1: No-starve readers-writers ----------------
void *p1_reader(void *id) {
    long i = (long)id;
    while (true) {
        // Allow readers and writers to take turns fairly using turnstile
        p1_turnstile->wait();
        p1_turnstile->signal();

        p1_mutex->wait();
        p1_readers++;
        if (p1_readers == 1) {
            p1_roomEmpty->wait(); // first reader locks room
        }
        p1_mutex->signal();

        // Readingg
        printf("Problem1 Reader %ld: reading\n", i);
        fflush(stdout);
        rand_sleep(0.5, 1.2);

        p1_mutex->wait();
        p1_readers--;
        if (p1_readers == 0) {
            p1_roomEmpty->signal(); // last reader frees room
        }
        p1_mutex->signal();

        rand_sleep(0.2, 0.8); // think/wait before next read
    }
    return NULL;
}

void *p1_writer(void *id) {
    long i = (long)id;
    while (true) {
        p1_turnstile->wait();       // get in line
        p1_roomEmpty->wait();      // wait until room empty (no readers)
        // Writing
        printf("Problem1 Writer %ld: writing\n", i);
        fflush(stdout);
        rand_sleep(0.8, 1.6);

        p1_roomEmpty->signal();
        p1_turnstile->signal();

        rand_sleep(0.3, 1.0);
    }
    return NULL;
}

// ---------------- Problem 2: Writer-priority readers-writers ----------------
void *p2_reader(void *id) {
    long i = (long)id;
    while (true) {
        // Writer priority: if a writer waits at turnstile, reader should not pass
        p2_turnstile->wait();
        // once admitted, reader proceeds to increment readers
        p2_mutex->wait();
        p2_readers++;
        if (p2_readers == 1) {
            p2_roomEmpty->wait(); // first reader locks room
        }
        p2_mutex->signal();
        p2_turnstile->signal();

        // Reading
        printf("Problem2 Reader %ld: reading\n", i);
        fflush(stdout);
        rand_sleep(0.5, 1.2);

        p2_mutex->wait();
        p2_readers--;
        if (p2_readers == 0) {
            p2_roomEmpty->signal(); // last reader frees room
        }
        p2_mutex->signal();

        rand_sleep(0.2, 0.8);
    }
    return NULL;
}

void *p2_writer(void *id) {
    long i = (long)id;
    while (true) {
        p2_turnstile->wait();   // block new readers from entering
        p2_roomEmpty->wait();   // wait for readers to leave
        // Writing
        printf("Problem2 Writer %ld: writing\n", i);
        fflush(stdout);
        rand_sleep(0.8, 1.6);

        p2_roomEmpty->signal();
        p2_turnstile->signal();

        rand_sleep(0.3, 1.0);
    }
    return NULL;
}

// ---------------- Problem 3: Dining Philosophers #1 (naive left then right) ----------------
void *p3_philosopher(void *id) {
    long i = (long)id; // 1..5
    int index = (int)(i - 1);
    int left = index;
    int right = (index + 1) % NUM_PHILOSOPHERS;
    while (true) {
        // Thinking
        printf("Problem3 Philosopher %ld: thinking\n", i);
        fflush(stdout);
        rand_sleep(0.5, 1.3);

        // pick up left then right (may deadlock)
        p3_chopstick[left].wait();
        printf("Problem3 Philosopher %ld: picked up left (%d)\n", i, left);
        fflush(stdout);
        rand_sleep(0.05, 0.2); // small delay to increase chance of interesting interleavings

        p3_chopstick[right].wait();
        printf("Problem3 Philosopher %ld: picked up right (%d) and eating\n", i, right);
        fflush(stdout);

        // Eating
        rand_sleep(0.6, 1.2);

        // Put down chopsticks
        p3_chopstick[right].signal();
        p3_chopstick[left].signal();
        printf("Problem3 Philosopher %ld: finished eating and put down chopsticks\n", i);
        fflush(stdout);
    }
    return NULL;
}

// ---------------- Problem 4: Dining Philosophers #2 (asymmetric) ----------------
void *p4_philosopher(void *id) {
    long i = (long)id; // 1..5
    int index = (int)(i - 1);
    int left = index;
    int right = (index + 1) % NUM_PHILOSOPHERS;
    while (true) {
        // Thinking
        printf("Problem4 Philosopher %ld: thinking\n", i);
        fflush(stdout);
        rand_sleep(0.5, 1.3);

        // Asymmetric pick-up strategy:
        // Even-indexed philosopher picks right then left (index 1,3,... when using 1-based id -> even id -> index odd)
        if ((i % 2) == 0) {
            // pick up right first
            p4_chopstick[right].wait();
            printf("Problem4 Philosopher %ld: picked up right (%d)\n", i, right);
            fflush(stdout);
            rand_sleep(0.02, 0.15);

            p4_chopstick[left].wait();
            printf("Problem4 Philosopher %ld: picked up left (%d) and eating\n", i, left);
            fflush(stdout);
        } else {
            // pick up left first
            p4_chopstick[left].wait();
            printf("Problem4 Philosopher %ld: picked up left (%d)\n", i, left);
            fflush(stdout);
            rand_sleep(0.02, 0.15);

            p4_chopstick[right].wait();
            printf("Problem4 Philosopher %ld: picked up right (%d) and eating\n", i, right);
            fflush(stdout);
        }

        // Eating
        rand_sleep(0.6, 1.2);

        // Put down chopsticks
        p4_chopstick[left].signal();
        p4_chopstick[right].signal();
        printf("Problem4 Philosopher %ld: finished eating and put down chopsticks\n", i);
        fflush(stdout);
    }
    return NULL;
}

// -------------- Runner functions for each problem --------------
void run_problem1() {
    // initialize semaphores
    p1_mutex = new Semaphore(1);
    p1_roomEmpty = new Semaphore(1);
    p1_turnstile = new Semaphore(1);
    p1_readers = 0;

    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];

    for (long i = 0; i < NUM_READERS; ++i) {
        pthread_create(&readers[i], NULL, p1_reader, (void*)(i+1));
    }
    for (long i = 0; i < NUM_WRITERS; ++i) {
        pthread_create(&writers[i], NULL, p1_writer, (void*)(i+1));
    }

    printf("Problem 1 (No-starve readers-writers) started with %d readers and %d writers.\n",
           NUM_READERS, NUM_WRITERS);
}

void run_problem2() {
    // initialize semaphores
    p2_mutex = new Semaphore(1);
    p2_roomEmpty = new Semaphore(1);
    p2_turnstile = new Semaphore(1);
    p2_readers = 0;

    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];

    for (long i = 0; i < NUM_READERS; ++i) {
        pthread_create(&readers[i], NULL, p2_reader, (void*)(i+1));
    }
    for (long i = 0; i < NUM_WRITERS; ++i) {
        pthread_create(&writers[i], NULL, p2_writer, (void*)(i+1));
    }

    printf("Problem 2 (Writer-priority readers-writers) started with %d readers and %d writers.\n",
           NUM_READERS, NUM_WRITERS);
}

void run_problem3() {
    // allocate an array of semaphores for chopsticks (initialized to 1)
    p3_chopstick = new Semaphore[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        // each default constructed to 0, so re-initialize by placement (destroy/re-init)
        // But our class only wraps sem_t and default ctor not provided with 1; so recreate manually:
        // We'll use signal to make it available once (since default ctor set 0)
        p3_chopstick[i].signal(); // set initial value to 1
    }

    pthread_t philosophers[NUM_PHILOSOPHERS];
    for (long i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_create(&philosophers[i], NULL, p3_philosopher, (void*)(i+1));
    }
    printf("Problem 3 (Dining Philosophers #1) started.\n");
}

void run_problem4() {
    p4_chopstick = new Semaphore[NUM_PHILOSOPHERS];
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        p4_chopstick[i].signal();
    }

    pthread_t philosophers[NUM_PHILOSOPHERS];
    for (long i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_create(&philosophers[i], NULL, p4_philosopher, (void*)(i+1));
    }
    printf("Problem 4 (Dining Philosophers #2 asymmetric) started.\n");
}

// -------------- main --------------
int main(int argc, char **argv) {
    srand((unsigned)time(NULL));
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <problem#>\n", argv[0]);
        fprintf(stderr, "  1 - No-starve readers-writers (5 readers, 5 writers)\n");
        fprintf(stderr, "  2 - Writer-priority readers-writers (5 readers, 5 writers)\n");
        fprintf(stderr, "  3 - Dining philosophers solution #1 (naive)\n");
        fprintf(stderr, "  4 - Dining philosophers solution #2 (asymmetric)\n");
        exit(EXIT_FAILURE);
    }

    int problem = atoi(argv[1]);
    switch (problem) {
        case 1: run_problem1(); break;
        case 2: run_problem2(); break;
        case 3: run_problem3(); break;
        case 4: run_problem4(); break;
        default:
            fprintf(stderr, "Invalid problem number: %d\n", problem);
            exit(EXIT_FAILURE);
    }

    // Let threads run forever (or until user stops the program with Ctrl-C).
    // Use pthread_exit to keep main alive while other threads run.
    pthread_exit(NULL);
    return 0;
}
