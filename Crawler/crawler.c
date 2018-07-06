#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "support.h"

pthread_mutex_t mtx, statistics;                // declare mutexes
pthread_cond_t cond_nonempty;                   // conditional vars
pthread_cond_t cond_nonfull;
pool_t pool;                                    // our queue

int main(int argc, char **argv) {
    int serving_port = 0, command_port = 0, numThreads = 0, i = 0;
    char *saveDir = NULL, *host_or_ip = NULL, *starting_URL = NULL;

    typedef struct{
        int port;
        char *url;
    }prod_args;


    // Check there are 12 arguments in total
    if (argc != 12){
        printf("Wrong number of arguments. Expected (12). Actual (%d)\n"
               "Correct Usage: ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n"
               "Press <ENTER> to continue.\n", argc);
        getchar();
        exit(EXIT_FAILURE);
    }

    // Get arguments
    for (i=1;i<argc;i+=2){                      // iterate all flag (odd) arguments
        if (!strcmp(argv[i], "-h")){            // if this is "-h" flag
            host_or_ip = argv[i+1];             // next arg is the server ip
            continue;
        }
        else if (!strcmp(argv[i],"-p")){        // if this is "-p" flag
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
            saveDir = (argv[i+1]);              // next arg is root dir
            continue;
        }
        else {
            starting_URL = argv[11];            // last argument is the starting URL
            continue;
        }
    }

    // Check they are all ok
    if (!saveDir || !serving_port || !command_port || !numThreads || !host_or_ip || !starting_URL){
        printf("Wrong arguments.\n"
               "Correct Usage: ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n"
               "Press <ENTER> to continue.\n");
        getchar();
        exit(EXIT_FAILURE);
    }

    pthread_t cons[numThreads], prod;                               // declare threads

    struct prod_args *prodArgs = malloc(sizeof(struct prod_args));  // Pass thread args as a struct
    prodArgs->port_c        = command_port;
    prodArgs->url           = starting_URL;
    prodArgs->domain        = host_or_ip;
    prodArgs->port_s        = serving_port;
    prodArgs->numThreads    = numThreads;
    prodArgs->saveDir       = saveDir;

    printf("Crawler is starting up...\n");

    initializePool(&pool);                                          // Initialize pool and mutexes
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&statistics, 0);

    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);

    pthread_create(&prod, 0, producer, (void *) prodArgs);          // Args must be cast to void *
    for(i = 0; i < numThreads; i++)
        pthread_create(&cons[i], 0, consumer, (void *) prodArgs);

    pthread_join(prod, 0);                                          // Join terminating threads
    for(i = 0; i < numThreads; i++)
        pthread_join(cons[i], 0);

    pthread_cond_destroy(&cond_nonempty);                           // Destroy conditionals and mutexes
    pthread_cond_destroy(&cond_nonfull);

    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&statistics);


    return 0;
}