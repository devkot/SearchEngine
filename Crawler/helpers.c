#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "helpers.h"
#include "trie.h"

int isDuplicate(WordSearchNode *head, char *word){
    WordSearchNode *cur = head;
    int found = 0;

    while (cur && !found) {
        if (!strcmp(cur->word, word))
            found = 1;
        cur = cur->next;
    }

    return found;
}

WordSearchNode *addWordSearchNode(WordSearchNode *head, char *fileName, int line, char *contents, int *numOfDocs, char *word){
    WordSearchNode *curNode = head, *prevNode = head, *newNode;
    int done = 0;

    if(!head){              // if list is empty
        head = (WordSearchNode *) malloc(sizeof(WordSearchNode));
        head->fileName = (char *) malloc(strlen(fileName) + 1);
        head->contents = (char *) malloc(strlen(contents) + 1);
        head->word     = (char *) malloc(strlen(word) + 1);

        strcpy(head->fileName, fileName);
        strcpy(head->contents, contents);
        strcpy(head->word, word);

        head->line     = line;
        head->next     = NULL;
        (*numOfDocs)++;
    }
    else{                   // if list is not empty
        while(!done){
            if((!curNode || !strcmp(curNode->fileName, fileName))){

                newNode = (WordSearchNode *) malloc(sizeof(WordSearchNode));
                newNode->fileName = (char *) malloc(strlen(fileName) + 1);
                newNode->contents = (char *) malloc(strlen(contents) + 1);
                newNode->word     = (char *) malloc(strlen(word) + 1);

                strcpy(newNode->fileName, fileName);
                strcpy(newNode->contents, contents);
                strcpy(newNode->word, word);

                newNode->line = line;
                newNode->next = curNode;

                if (head == curNode)
                    head = newNode;
                else
                    prevNode->next = newNode;
                (*numOfDocs)++;
                done = 1;
            }
            else if(strcmp(curNode->fileName, fileName) != 0){      // we still need to traverse the list
                prevNode = curNode;
                curNode = curNode->next;
            }
            else if(!strcmp(curNode->word, word))
                done = 1;
            else
                done = 1;
        }
    }
    return head;
}

void destroyWordSearchNodes(WordSearchNode *head){
    WordSearchNode *curNode = head, *prevNode = head;

    while (curNode){
        curNode = curNode->next;
        free(prevNode->word);
        free(prevNode->fileName);
        free(prevNode->contents);
        free(prevNode);
        prevNode = curNode;
    }
}

// Reads the desired line to return to the search query
char *readLine(char *fileName, int lineNumber){
    FILE *file;
    char lineBuffer[100000], *line = NULL;
    int curLine = 0;

    file = fopen(fileName, "r");                        // Open file

    if (file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while(fgets(lineBuffer, 99999, file)) {
        if(curLine == lineNumber) {                     // read the line we need
            line = (char *) malloc(strlen(lineBuffer) + 1);
            strcpy(line, lineBuffer);                   // copy our desired line from buffer
            break;
        }
        curLine++;
    }

    fclose(file);
    return line;
}