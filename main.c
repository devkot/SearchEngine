#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "functions.h"

int bytes = 0, pages = 0;
pthread_mutex_t mtx, statistics;                // declare mutexes
pthread_cond_t cond_nonempty;                   // conditional vars
pthread_cond_t cond_nonfull;
pool_t pool;                                    // our queue

int main(int argc, char **argv) {
    int serving_port = 0, command_port = 0, numThreads = 0, i = 0;
    char *rootDir = NULL;

    // Check there are 5 arguments in total
    if (argc != 9){
        printf("Wrong number of arguments. Expected (9). Actual (%d)\n"
               "Correct Usage: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n"
               "Press <ENTER> to continue.\n", argc);
        getchar();
        exit(EXIT_FAILURE);
    }

    // Get arguments
    for (i=1;i<argc;i+=2){                      // iterate all flag (odd) arguments
        if (!strcmp(argv[i],"-p")){             // if this is "-p" flag
            serving_port = atoi(argv[i+1]);     // next arg is serving port
            continue;
        }
        else if (!strcmp(argv[i],"-c")){        // if this is "-d" flag
            command_port = atoi(argv[i+1]);     // next arg is command port
            continue;
        }
        else if (!strcmp(argv[i], "-t")){       // if this is "-t" flag
            numThreads = atoi(argv[i+1]);       // next arg is num of threads
            continue;
        }
        else if (!strcmp(argv[i], "-d")){       // if this is "-d" flag
            rootDir = (argv[i+1]);              // next arg is root dir
            continue;
        }
    }

    // Check they are all ok
    if (!rootDir || !serving_port || !command_port || !numThreads ){
        printf("Wrong arguments.\n"
               "Correct Usage: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n"
               "Press <ENTER> to continue.\n");
        getchar();
        exit(EXIT_FAILURE);
    }

    pthread_t cons[numThreads], prod, comm;                     // declare threads

    printf("Server is booting up...\n");

    initialize(&pool);                                          // Initialize pool and mutexes
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&statistics, 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);

    pthread_create(&prod, 0, producer, (void *) &serving_port); // Args must be cast to void *
    pthread_create(&comm, 0, command, (void *) &command_port);
    for(i = 0; i < numThreads; i++)
        pthread_create(&cons[i], 0, consumer, 0);

    pthread_join(prod, 0);                                      // Join terminating threads
    pthread_join(comm, 0);
    for(i = 0; i < numThreads; i++)
        pthread_join(cons[i], 0);

    pthread_cond_destroy(&cond_nonempty);                       // Destroy conditionals and mutexes
    pthread_cond_destroy(&cond_nonfull);

    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&statistics);

    return 0;
}