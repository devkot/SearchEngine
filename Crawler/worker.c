#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

#include "trie.h"
#include "functions.h"
#include "helpers.h"

#define MSGSIZE 10000

int main(int argc, char **argv) {
    int i, fdIn, fdOut, bytesToRead = 0, nWords = 0, nChars = 0, nLines = 0, flag = 0, queries = 0;
    char *workerID, pipeIn[50], pipeOut[50], temp[5], msgbuf[MSGSIZE + 1], *lineBuffer;
    // In and Out follow jobExecutor's order

    TrieNode *trie = createTrieNode();
    // the Trie data structure that will contain all the words

    if (argc != 2) {                    // Two arguments for worker + i
        printf("Wrong number of arguments used in execlp()\n");
        exit(EXIT_FAILURE);
    }

    i = atoi(argv[1]);                  // Get the pipe ID corresponding to this worker

    workerID = (char *) malloc(strlen(argv[0]) + strlen(argv[1]));  // Allocate memory for the worker ID

    strcat(workerID, argv[0]);
    strcat(workerID, argv[1]);

    sprintf(pipeIn, "/tmp/fifo_%d_in", i);
    sprintf(pipeOut, "/tmp/fifo_%d_out", i);

    if ((fdIn = open(pipeIn, O_NONBLOCK | O_RDWR)) == -1)
        perror("open");
    else
        flag = 1;

    if ((fdOut = open(pipeOut, O_NONBLOCK | O_RDWR)) == -1)
        perror("open");

    if (flag == 1) {                                   // Initialize data structures
        while ((read(fdIn, temp, 4)) > 0) {            // Read 4 byte digits to determine the payload's size
            *(temp + 4) = '\0';                        // Append terminating character
            bytesToRead = atoi(temp);

            lineBuffer = (char *) malloc(sizeof(char) * bytesToRead);

            if ((read(fdIn, lineBuffer, bytesToRead)) == -1)      // Read the payload
                perror("Error reading docfile");

            *(lineBuffer + bytesToRead) = '\0';

            readFolder(lineBuffer, trie, &nWords, &nChars, &nLines);        // Get path contents

            flag = 0;
        }
    } else
        exit(EXIT_FAILURE);

    //signal(SIGALRM, alarmHandler);

    while (1) {
        while ((read(fdIn, msgbuf, MSGSIZE + 1)) <= 0) {            // Read query command, maxcount is the longest so we read 8 bytes
        }
        msgbuf[MSGSIZE] = '\0';

        if (strstr(msgbuf, "search")) {                    // if this is a search command
            queries += searchQuery(msgbuf + 7, trie, 1, fdOut);
        }
        else if (strstr(msgbuf, "exit")) {                 // Exit
            break;
        }
    }

    // Close pipes and free memory
    close(fdIn);
    close(fdOut);

    destroyTrie(trie);

    // Exit with the number of queries
    exit(queries);
}