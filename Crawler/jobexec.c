#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "jobexec.h"

extern int *fdIn, *fdOut, *pids;

DocumentNode *prepareJobExec(char *docfile, int numWorkers, int *nDocs) {
    int i, j, remainder = 0, increment = 0, *incrementArray;
    char lineBuffer[100000], *buffer = NULL;
    char pipeOut[50], pipeIn[50];
    DocumentNode *documentArray;
    FILE *inputFile;

    // open input file to create data structures
    inputFile = fopen(docfile, "r");
    if (inputFile == NULL) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    // count how many documents the input file has
    while (fgets(lineBuffer, 99999, inputFile))
        (*nDocs)++;
    rewind(inputFile);

    printf("Allocating %d cells for the array\n", *nDocs);
    // allocate nDocs cells for the array of the documents
    documentArray = (DocumentNode *) malloc(sizeof(DocumentNode) * (*nDocs));

    // initialize all the data structures with the contents of the input file
    if (!initialize(inputFile, documentArray)) {
        printf("Wrong document line read.\n");
        destroyDocuments(documentArray, *nDocs);
        printf("Successfully destroyed.\n");
        exit(EXIT_FAILURE);
    }
    fclose(inputFile);

    if (numWorkers > *nDocs)         // in case we have more workers than files
        numWorkers = *nDocs;         // set worker number equal to lines (each gets one)


// handler for sigchild
//signal(SIGCHLD, handler);

    fdIn = (int *) malloc(sizeof(int) * numWorkers);
    fdOut = (int *) malloc(sizeof(int) * numWorkers);

    for (i = 0; i < numWorkers; i++) {
        sprintf(pipeIn, "/tmp/fifo_%d_in", i);
        mkfifo(pipeIn, S_IRWXU);
        fdIn[i] = open(pipeIn, O_NONBLOCK | O_RDWR);

        sprintf(pipeOut, "/tmp/fifo_%d_out", i);
        mkfifo(pipeOut, S_IRWXU);
        fdOut[i] = open(pipeOut, O_NONBLOCK | O_RDWR);
    }

    pids    = (int *) malloc(sizeof(int) * numWorkers);
    buffer  = (char *) malloc(sizeof(char) * numWorkers);

    increment = determineWorkload(*nDocs, numWorkers, &remainder);

    incrementArray = splitWorkload(increment, &remainder, numWorkers);

    distributeDocs(documentArray, incrementArray, numWorkers, fdIn);

    for (j = 0; j < numWorkers; j++) {
        pids[j] = fork();

        if (pids[j] == -1) {
            perror("fork");
            exit(1);
        } else if (pids[j] == 0) {           // child
            sprintf(buffer, "%d", j);
            if (execlp("./worker", "worker", buffer, NULL) == -1) {
                perror("execlp");
                exit(EXIT_FAILURE);
            }
            exit(0);
        } else {                            // parent
            //printf("Im the parent with pid %d\n", getpid());
        }
    }

    return documentArray;
}

void cleanUp(int numWorkers, DocumentNode *documentArray, int nDocs) {
    int i;
    char pipeOut[50], pipeIn[50];

    for (i = 0; i < numWorkers; i++) {
        sprintf(pipeIn, "/tmp/fifo_%d_in", i);
        sprintf(pipeOut, "/tmp/fifo_%d_out", i);

        close(fdIn[i]);
        close(fdOut[i]);

        unlink(pipeIn);
        unlink(pipeOut);
    }

    destroyDocuments(documentArray, nDocs);
}