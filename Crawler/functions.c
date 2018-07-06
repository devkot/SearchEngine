#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>

#include "functions.h"
#include "trie.h"
#include "helpers.h"

#define MSGSIZE 10000

int alarmExpired = 0;

void readFolder(char *path, TrieNode *trie, int *nWords, int *nChars, int *nLines){
    DIR *dp;
    struct dirent *dir;
    char *newName;
    if((dp = opendir(path)) == NULL){                       // Open directory
        perror("readfolder");
        return;
    }
    while((dir = readdir(dp)) != NULL){                     // Read contents till the end
        if(dir->d_ino == 0)                                 // Ignore removed entries
            continue;
        newName = malloc(strlen(path)+strlen(dir->d_name)+2);
        strcpy(newName, path);                              // Compose pathname

        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ) { //Filter out
            //do nothing
        }
        else{
            strcat(newName, dir->d_name);
            readFile(newName, trie, nWords, nChars, nLines);
        }

        free(newName);
    }
    closedir(dp);                                           // Close directory
}

int readFile(char *inputFile, TrieNode *trie, int *nWords, int *nChars, int *nLines){
    char lineBuffer[100000], *word, whitespaces[3] = " \t";
    char tags[100] = "<html></html><head></head><body></body><!DOCTYPE html></a><a href html";
    int lineNumber = 0;
    FILE *file;
    file = fopen(inputFile, "r");                           // Open file

    if (file == NULL){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    while(fgets(lineBuffer, 99999, file)){
        (*nChars) += strlen(lineBuffer);                    // calculate total file characters (with whitespaces and \n)

        strtok(lineBuffer, "\n");                           // remove the useless \n character at the end of the line

        word = strtok(lineBuffer, whitespaces);             // feed strtok

        if(!strstr(tags,word)) {                            // filter out tags
            insertWord(trie, word, lineNumber, inputFile);  // insert to trie
            (*nWords)++;                                    // increase the total number of words of Trie
        }

        while ((word = strtok(NULL, whitespaces))){         // start reading the document word by word
            if(!strstr(tags,word)){                         // filter out tags
//                continue;

                insertWord(trie, word, lineNumber, inputFile);  // insert to trie (filename, line, n)
                (*nWords)++;                                    // increase the total number of words of Trie
//			    docWords++;                                     // increase the total number of words of current document
            }
        }
        lineNumber++;

    }
    (*nLines) += lineNumber;
    fclose(file);

}

// performs the queries of the user
int searchQuery(char *query, TrieNode *root, int deadline, int fdOut){
	char *word, *line, payload[MSGSIZE+1];
	TrieNode *wordNode;
	QueryWordInfoNode *wordArray;
	WordSearchNode *docHead = NULL;
	PostingListNode *postingListNode;
	int wordCounter = 0, wordFoundCounter = 0, numOfDocs = 0, numOfWords, queries = 0;

    numOfWords = countWords(query);       // count number of words that will be used for the query

    if (!numOfWords){
        return 1;
    }

    wordArray = (QueryWordInfoNode *) malloc(sizeof(QueryWordInfoNode) * numOfWords); // allocate at most 10 cells for this array

	word = strtok(query, " \t");
    while (wordCounter++ < numOfWords){                         // read every word of the query (up to 10 actually)
        if (!isDuplicate(docHead, word)) {
            wordArray[wordFoundCounter].wordNode = wordNode = searchTrie(root, word);    // add word's trie node to our list
            wordArray[wordFoundCounter++].word = word;                                   // add the word string to our list

            if (wordNode) {                                     // if word actually exists in trie
                postingListNode = wordNode->postingList;        // get it's posting list

                while (postingListNode) {                       // while there are nodes
                    line = readLine(postingListNode->fileName, postingListNode->lineNumber);
                    docHead = addWordSearchNode(docHead, postingListNode->fileName, postingListNode->lineNumber, line,
                                                &numOfDocs, word);
                    
                    sprintf(payload, "Line: %d File: %s Text: %s", postingListNode->lineNumber, postingListNode->fileName, line);
                    writeBack(fdOut, payload);
                    queries++;
                    
                    postingListNode = postingListNode->next;    // go to next node
                }
            }
        }
        word = strtok(NULL, " \t");
    }

    int worked = 0;
    strcpy(payload, "$$$TERMINATE$$$");
    while((worked = write(fdOut, payload, MSGSIZE+1)) <= 0){}

    if(worked == -1)
        perror("write");

    free(wordArray);
    destroyWordSearchNodes(docHead);

    return queries;
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

int wordCount(int *nWords, int *nChars, int *nLines, int fdOut){
    char msgbuf[MSGSIZE+1];

    sprintf(msgbuf, "%d %d %d", *nChars, *nWords, *nLines);

    write(fdOut, msgbuf, MSGSIZE+1);

}

void writeBack(int fdOut, char *payload){
    char msgbuf[MSGSIZE + 1];
    strcpy(msgbuf,payload);

    while((write(fdOut, msgbuf, MSGSIZE+1)) <= 0){}
}
