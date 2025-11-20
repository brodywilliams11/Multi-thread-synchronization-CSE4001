//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};




/* global vars */
const int bufferSize = 5;
const int numConsumers = 3; 
const int numProducers = 3; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             



/*
    Producer function 
*/
void *Writer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        
            printf("Writer %d Writing...\n", x);
            fflush(stdout);
        
    }

}

/*
    Consumer function 
*/
void *Reader ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        
            printf("Reader %d Reading... \n", x);
            fflush(stdout);
        
        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }

}


int main(int argc, char **argv )
{
    pthread_t producerThread[ numProducers ];
    pthread_t consumerThread[ numConsumers ];

    pthread_t writerThread[ numProducers ];
    pthread_t readerThread[ numConsumers ];

    // Create the producers 
    for( long p = 0; p < 3; p++ )
    {
        int rc = pthread_create ( &writerThread[ p ], NULL, 
                                  Writer, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating writer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < 2; c++ )
    {
        int rc = pthread_create ( &readerThread[ c ], NULL, 
                                  Reader, (void *) (c+1) );
        if (rc) {
            printf("ERROR creating reader thread # %d; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    printf("Main: program completed. Exiting.\n");


    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL); 


} /* main() */






