#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utilities.h"
#include "support.h"

#define MSGSIZE 10000

int initialize(FILE *fileName, DocumentNode *documentArray){
    int maxLength = 1, curLength = 0, lineId = 0, docWords;
    char lineBuffer[100000], *newDoc;

    // start reading the file until EOF is read
    while(fgets(lineBuffer, 99999, fileName)){                  // read docfile contents
        strtok(lineBuffer, "\n");                               // remove the useless \n character at the end of the line

        curLength = strlen(lineBuffer);
        if( curLength > maxLength)
            maxLength = curLength;

        // every document will have the length of the line read
        newDoc = (char *) malloc(strlen(lineBuffer) +1);
        // copy the useful content (just the text) of lineBuffer to newDoc
        strcpy(newDoc, lineBuffer);

        docWords = 1;                                           // init at 1 so we don't have to increment it for this instance
        strtok(lineBuffer, "/");

        while((strtok(NULL, "/"))){                             // count the path's words
            docWords++;
        }

        addDocument(documentArray, lineId, newDoc, docWords);   // add document to our array
        lineId++;
    }
    return maxLength;
}

void addDocument(DocumentNode *documentArray, int pos, char *text, int nWords){
    documentArray[pos].text = text;
    documentArray[pos].nWords = nWords;
}

void destroyDocuments(DocumentNode *documentNodes, int nLines){
    int i;

    for (i = 0;i < nLines; i++)
        free(documentNodes[i].text);
    free(documentNodes);
}

int determineWorkload(int nDocs, int numWorkers, int *remainder){
    int increment = 0;

    if ((nDocs % numWorkers) == 0){
        increment = nDocs / numWorkers;            // determine how many files each worker will get
    }
    else if ((nDocs % numWorkers) > 0){
        increment = nDocs / numWorkers;
        (*remainder) = nDocs % numWorkers;
    }

    return increment;
}

int *splitWorkload(int increment, int *remainder, int numWorkers) {
    int i, *workerLoad;

    workerLoad = (int *) malloc(sizeof(int) * numWorkers);

    for(i = 0; i < numWorkers; i++){
        if(*remainder > 0) {
            workerLoad[i] = increment + 1;          // split the remainder between the workers
            (*remainder)--;
        }
        else
            workerLoad[i] = increment;
    }

    return workerLoad;
}

void distributeDocs(DocumentNode *documentArray, int *incrementArray, int numWorkers, int *fdIn){
    int i, j, lines = 0;
    char tempSize[5], sizeTriple[4] = "000", sizeDouble[3] = "00", sizeSimple[2] = "0";

    for(i = 0; i < numWorkers; i++){
        for(j = 0; j < incrementArray[i]; j++){
            // Pipe atomicity is guaranteed at 4096 bytes max so make the size payload contain 4 digits
            if(strlen(documentArray[lines].text) < 10 )
                sprintf(tempSize, "%s%d",sizeTriple, (int) strlen(documentArray[lines].text));
            else if(strlen(documentArray[lines].text) < 100)
                sprintf(tempSize, "%s%d", sizeDouble, (int) strlen(documentArray[lines].text));
            else if(strlen(documentArray[lines].text) < 1000)
                sprintf(tempSize, "%s%d", sizeSimple, (int) strlen(documentArray[lines].text));

            if((write(fdIn[i], tempSize, strlen(tempSize))) == -1)
                perror("write");

            if((write(fdIn[i], documentArray[lines].text, strlen(documentArray[lines].text))) == -1)
                perror("write");

            lines++;
        }
    }
}

void writeQuery(char *payload, char *command, int numWorkers, int *fdIn, int *fdOut, int sock){
    char msgSearch[MSGSIZE+1], temp[MSGSIZE+1], receiveSearch[MSGSIZE+1], msgExit[MSGSIZE+1];
    int childPid, queries = 0, i, status = 0, numOfWords;

        if(!strcmp(command, "search")){                                 // If it's a search commmand

            numOfWords = countWords(payload);       // count number of words that will be used for the query
            if (!numOfWords){
                printMenu(1);
                return;
            }

            wipeBuffer(msgSearch);                                      // Cleaning buffers
            wipeBuffer(receiveSearch);
            for(i = 0; i < numWorkers; i++) {
                strcpy(temp, command);
                strcat(temp, " ");
                strcat(temp, payload);
                strcpy(msgSearch, temp);

                while ((write(fdIn[i], msgSearch, MSGSIZE + 1)) <= 0){
                    if(strstr(receiveSearch, "$$$TERMINATE$$$"))
                        return;                                          // Added on project3
                }  // Transmit command
                wipeBuffer(receiveSearch);                               // Wipe buffer to remove terminating keyword
// TODO write to socket instead of printing
                while (!strstr(receiveSearch, "$$$TERMINATE$$$")) {
                    while ((read(fdOut[i], receiveSearch, MSGSIZE + 1)) <= 0) {
                    }
                    receiveSearch[MSGSIZE] = '\0';
                    if(!strstr(receiveSearch, "$$$TERMINATE$$$"))
                        customWrite(sock, receiveSearch, strlen(receiveSearch));
                }
            }

        }
        if(!strcmp(command, "exit")){                                          // Exit command

            wipeBuffer(msgExit);
            for(i = 0; i < numWorkers; i++){

                strcpy(msgExit, "exit");

                while((write(fdIn[i], msgExit, MSGSIZE+1)) <= 0){}            // Send exit command to workers and wait

                childPid = wait(&status);
                queries = status >> 8;

                printf("Child with pid: %d exited having found %d words\n", childPid, queries);
            }
        }
}

// count the number of words used in query (at most 10)
int countWords(char *query){
    int counter = 0, queryLength = strlen(query), i = 0;

    if (!strcmp(query, ""))
        return 0;

    while (i++ <= queryLength && counter < 10){
        if (*query == ' ' || *query == '\t' || *query == '\0')
            counter++;      // increase the number of words
        query++;            // go to next character
    }
    return counter;
}

// print the menu
void printMenu(int hasError){
    if (hasError)
        printf("Wrong Command given.");
    printf("Available commands:\n1) /search q1 .... q10 -d deadline\n2) /mincount word\n3) /maxcount word\n4) /wc\n5) /exit\n\n");
}

void wipeBuffer(char *buffer){
    memset(buffer, 0, 1);
}