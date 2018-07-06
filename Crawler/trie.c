#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"

// -------------------------------- Trie functions --------------------------------------------

// creates a new TrieNode and returns it
TrieNode * createTrieNode(){
    TrieNode *newNode = (TrieNode*) malloc(sizeof(TrieNode)); // allocate space for new node

    newNode->frequency   = 0;    // default frequency of a node is 0, since it's not a word (leaf)
    newNode->postingList = NULL; // no posting list for a non leaf node
    newNode->charList    = NULL; // no child (letter) nodes yet
    newNode->nDocs       = 0;    // no documents include this substring yet (unknown at the time if this is a leaf node)
    return newNode;
}

// inserts a new word in Trie and returns the trieNode
TrieNode * insertWord(TrieNode *root, char* word, int lineNumber, char *fileName){
    TrieNode *current = root;
    int newDoc;

    while (*word){                                     // while not all the string has been consumed
        current = findNextTrieNode(current, *word, 0); // go to the trie node that contains the current letter
        word++;                                        // point to next letter
    }

    current->frequency++;                              // increase the frequency of this word (current is pointing to the leaf node (aka where the word is)
    if (current->postingList == NULL){                 // if there is no posting list yet for this word
        current->postingList = createPostingList(lineNumber, fileName); // create it
        newDoc = 1;
    }
    else
        current->postingList = addToPostingList(current->postingList, lineNumber, &newDoc, fileName); // insert the doc id to the beginning of the inverted index of the word

    current->nDocs += newDoc;                           // will be incremented by one, if this was the first appearance of the word in the lineNumber

    return current;
}

// finds and creates (if necessary) next trie node for a word during insertion
// searchMode = 1 -> only search for the letter, do not create if not exists
// searchMode = 0 -> create next node if not exists
TrieNode * findNextTrieNode(TrieNode *root, char c, int searchMode){
    int done = 0;
    TrieNodeList *curNode = root->charList, *prevNode = root->charList, *newNode;

    if (!curNode){                          // if our trie node does not have a list of child nodes
        if (searchMode)                     // if we are simply searching for a word
            return NULL;                    // return NULL
        root->charList = curNode = newNode = (TrieNodeList *) malloc(sizeof(TrieNodeList)); // create the first node of the list
        newNode->letter = c;                // assign the letter
        newNode->next   = NULL;             // next pointer is null
        newNode->node   = createTrieNode(); // create the actual trie node
    }
    else{                                   // if our trie node has at least one child node
        while (!done){                      // start iterating the list of nodes
            if (!curNode || c < curNode->letter){ // if the current node corresponds to a letter bigger than the one we want
                if (searchMode)                   // if we are simply searching for a word
                    return NULL;                  // return NULL

                newNode = (TrieNodeList *) malloc(sizeof(TrieNodeList));
                newNode->letter = c;              // assign the letter
                newNode->next = curNode;          // next node of the new node is the current node
                newNode->node = createTrieNode(); // create the actual trie node
                if (prevNode != curNode)          // if the new node is not actually going to be the first node
                    prevNode->next = newNode;     // previous node will point to current node
                else                              // if this is the new head of the list
                    root->charList = newNode;     // make sure it now points to the new node
                curNode = newNode;                // store it here to return it at the end of the function
                done = 1;
            }
            else if (c == curNode->letter){ // if the node we are looking for already exists
                done = 1;
            }
            else{                         // if the current node corresponds to a letter lower than the one we want
                prevNode = curNode;       // go to next node
                curNode = curNode->next;  // go to next node
            }
        }
    }

    return curNode->node;
}

// Returns the trie node a word corresponds to, otherwise NULL
TrieNode * searchTrie(TrieNode *root, char *word){
    TrieNode *curNode = root;

    if (!word)
        return NULL;

    while (*word && curNode){                          // while not all the string has been consumed
        curNode = findNextTrieNode(curNode, *word, 1); // go to the trie node that contains the current letter
        word++;                                        // go to next letter
    }

    return curNode;
}

 // returns the posting list (if it exists) of a word
PostingListNode * getPostingList(TrieNode *root, char *word){
    TrieNode *node = searchTrie(root, word);

    if (node)
        return node->postingList;
    else
        return NULL;
 }

// recursively destroy Trie data structure
void destroyTrie(TrieNode *trieNode){
    TrieNodeList *cur, *prev;

    if (!trieNode)
        return;

    cur = prev = trieNode->charList;

    while (cur){
        destroyTrie(cur->node);
        cur = cur->next;
        free(prev);
        prev = cur;
    }

    destroyPostingList(trieNode->postingList);
    free(trieNode);
}

// ------------------------------- Posting List functions -------------------------------------------

// creates a new posting list for a leaf node of trie structure
PostingListNode * createPostingList(int lineNumber, char *fileName){

    PostingListNode *newNode = (PostingListNode*) malloc(sizeof(PostingListNode));
    newNode->fileName = (char *) malloc(strlen(fileName) + 1);

    newNode->lineNumber = lineNumber;
    newNode->n          = 1;
    //    newNode->fileName   = fileName;
    strcpy(newNode->fileName, fileName);
    newNode->next       = NULL;

    return newNode;
}

// adds a new posting list node (if necessary) and returns the new head of the list
// *newDoc = 1 - First time the word is included in lineNumber
// *newDoc = 0 - NOT the first time the word is included in lineNumber
PostingListNode * addToPostingList(PostingListNode *head, int lineNumber, int *newDoc, char *fileName){
    PostingListNode *newNode, *lineNode;

    if(!strcmp(head->fileName, fileName) && (head->lineNumber == lineNumber)) {
        (head->n)++;            // simply increase frequency

        *newDoc = 0;            // do not count this doc as new doc the word has appeared into

        return head;
    }
    else if (!strcmp(head->fileName, fileName) && (head->lineNumber != lineNumber)){  // if this is not the first time the word is found in the document /head->fileName == fileName
        lineNode = (PostingListNode *) malloc(sizeof(PostingListNode));
        lineNode->fileName = (char *) malloc(strlen(fileName) + 2);

        lineNode->lineNumber = lineNumber;
        lineNode->n = head->n + 1;
        strcpy(lineNode->fileName, fileName);
        lineNode->next       = head;

        *newDoc = 0;            // do not count this doc as new doc the word has appeared into

        return lineNode;            // head remains as is
    }
    else{                       // if this is the first time the word is found in the document
        newNode = (PostingListNode *) malloc(sizeof(PostingListNode));
        newNode->fileName = (char *) malloc(strlen(fileName) + 2);

        newNode->lineNumber = lineNumber;   // node points to the document the word is in
        newNode->n          = 1;            // frequency of the word inside the document is 1, since it is its first appearance
//        newNode->fileName   = fileName;     // set the fileName
        strcpy(newNode->fileName, fileName);
        newNode->next       = head;         // next node of the new node is the old head

        *newDoc = 1;            // make sure we count this doc in the unique docs the word has appeared

        return newNode;         // new node becomes the new head of the list
    }
}

// get the frequency of a given word in a given file name
int getDocFrequency(PostingListNode *head, char *fileName){
    PostingListNode *curNode = head;

    while (curNode){                     // while there are nodes to search

        if (!strcmp(fileName,curNode->fileName))    // if doc node is found
            return curNode->n;           // return frequency
        else if (strcmp(fileName,curNode->fileName) != 0) // if there are more nodes to search
            curNode = curNode->next;     // go to next
        else                             // else (there are more nodes actually, but since the list is in DESCENDING ORDER, there is no point to look further)
            return 0;                    // return zero (no such word found in specific doc id)
    }

    return 0;                            // default zero return value
}

// recursively destroy PostingList data structure
void destroyPostingList(PostingListNode *postingListNode){
    if (!postingListNode)
        return;

    destroyPostingList(postingListNode->next);
    free(postingListNode->fileName);
    free(postingListNode);
}
