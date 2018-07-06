#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <netdb.h>
#include <dirent.h>

#include "support.h"
#include "jobexec.h"

int pages = 0, bytes = 0, ready = 0;
int *fdIn, *fdOut, *pids;

extern pthread_mutex_t mtx, statistics;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonfull;
extern pool_t pool;
time_t start;

// from eclass
void initializePool(pool_t *pool) {
    pool->start = 0;
    pool->end   = -1;
    pool->count = 0;
}

// Place fd in the pool (eclass)
void place(pool_t *pool, char *url) {

    pthread_mutex_lock(&mtx);

    while (pool->count >= POOL_SIZE){
//        printf("Pool is Full\n");
        pthread_cond_wait(&cond_nonfull, &mtx);
    }
    pool->end = (pool->end + 1) % POOL_SIZE;

    strcpy(pool->url[pool->end], url);
    pool->count++;

    pthread_mutex_unlock(&mtx);
}

// Get fd from the pool (from eclass)
char  *obtain(pool_t *pool) {
    char *url;

    pthread_mutex_lock(&mtx);

    while (pool->count <= 0){
//        printf("Pool is Empty\n");
        ready++;
        pthread_cond_wait(&cond_nonempty, &mtx);
    }
    url = pool->url[pool->end];

    pool->start = (pool->start + 1) % POOL_SIZE;
    pool->count--;
    pthread_mutex_unlock(&mtx);

    return url;
}

// Thread responsible for listening requests
void *producer(void *ptr) {
    int command_port, serving_port, lsock, csock, nDocs = 0;
    char *url, *trimpath, *path, portString[4], telbuf[20], stats[200], *command, runTime[100], *saveDir;
    struct sockaddr_in myaddr;
    DocumentNode *docArray;

    time (&start);

//    serving_port = *((int *) ptr);

    struct prod_args* prodArgs = (struct prod_args*) ptr;           // Get arguments
    command_port = prodArgs->port_c;
    serving_port = prodArgs->port_s;
    url = (char *) malloc (strlen(prodArgs->url) + 1);
    strcpy(url, prodArgs->url);
    saveDir = (char *) malloc (strlen(prodArgs->saveDir) + 4);
    strcpy(saveDir, "./");
    strcat(saveDir, prodArgs->saveDir);
    strcat(saveDir, "/");

    sprintf(portString, "%d", serving_port);                        // Port to string
    trimpath = strstr(url, portString);                             // Split string after port
    path = (trimpath+strlen(portString));                           // Set pointer after port
    strcpy(url, path);

    printf("Beginning crawling...\n");
    place(&pool, url);                                              // Place the given URL into the pool

    pthread_cond_signal(&cond_nonempty);

    // TODO fork here


    if((lsock = socket( AF_INET, SOCK_STREAM, 0)) < 0)              // Create command socket
        perror_exit("socket");

    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(command_port);
    myaddr.sin_family = AF_INET;

    // Resolve address already in use
    if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &myaddr, sizeof(myaddr)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if(bind(lsock, (struct sockaddr *)&myaddr, sizeof(myaddr)))
        perror_exit("command");

    if(listen(lsock, 5) != 0)
        perror_exit("listen");
    printf("Crawling in progress...\n");
    usleep(5000000);                                      // Wait for 5 secs
//    while(dirExists(saveDir) == -1){}
//    while(pool.count > 0){}
//    while(ready < prodArgs->numThreads){}               // Wait for dir to get created

    printf("Trying to create %s ...\n", prodArgs->saveDir);
    readSaveFolder(saveDir);                                // Populate docfile
    printf("%s created!\n", prodArgs->saveDir);

    printf("Crawling finished...\n");

    docArray = prepareJobExec("docfile", prodArgs->numThreads, &nDocs);

    // TODO distribute docs here
    // TODO create more files

    printf("Crawler is ready to receive commands! <STATS, SHUTDOWN, SEARCH q1 q2...q10> via telnet\n");
    while(1){
        if((csock = accept(lsock, NULL, NULL)) < 0)      // Accept telnet connections
            perror_exit("accept");

        printf("Crawler received a telnet command...\n");
        bzero(telbuf, strlen(telbuf));
        //TODO make permanent connection
        if((read(csock, telbuf, 20)) < 0)
            perror_exit("Reading request");

        telbuf[200] = '\0';                              // Append terminating character

        strtok(telbuf, "\r\n");                          // Remove chars appended by telnet

        command = strtok(telbuf, " ");

        if (!strcmp(command, "STATS")){
            bzero(stats, 200);
            sprintf(stats, "Crawler up for %s, downloaded %d pages, %d bytes\n", getRunTime(runTime, start), pages, bytes);
            customWrite(csock, stats, strlen(stats));
        }
        else if (!strcmp(command, "SHUTDOWN")){ // TODO something bugs here
            bzero(stats, 200);
            sprintf(stats, "Server shutting down...\n");
            customWrite(csock, stats, 25);
            writeQuery(telbuf + 7, "exit", 4, fdIn, fdOut, csock);
            cleanUp(prodArgs->numThreads, docArray, nDocs);
            close(csock);
            exit(0);
        }
        else if (!strcmp(command, "SEARCH")){
            bzero(stats, 200);
            if (pool.count != 0) {
                sprintf(stats, "Crawling in progress. Please try again...\n");
                customWrite(csock, stats, 43);
            }
            writeQuery(telbuf + 7, "search", 4, fdIn, fdOut, csock);
            //TODO send command
            // TODO bigger buffers
            // TODO telnet send back response with sprintf
            // TODO communicate with pipes...hmmm?
        }
        else {
            bzero(stats, 200);
            sprintf(stats, "Invalid command\n");
            customWrite(csock, stats, 17);
        }
    }
}

// Get fds from the pool and serve them to the user
void *consumer(void *ptr) {
    char *domain, *url, *path, *saveDir;
    char portString[4], dummyBuffer[1500], *dummyFile;
    int port, sock, bytesReadFirst, contLength, bytesToRead, headerSize, bytesRead, bytesWrote;
    struct sockaddr_in servadd;                                                 // Server address
    struct hostent *hp;                                                         // Resolve server IP
    FILE *file;

    struct prod_args* prodArgs = (struct prod_args*) ptr;                       // Get arguments
    port = prodArgs->port_s;                                                    // Avoid redundant parsing
    domain = prodArgs->domain;
    saveDir = (char *) malloc (strlen(prodArgs->saveDir) + 4);
//    strcpy(saveDir, "./");
    strcat(saveDir, prodArgs->saveDir);
//    strcat(saveDir, "/");


    if((hp = gethostbyname(prodArgs->domain)) == NULL){
        herror("gethostbyname");
        exit(1);
    }

    memset(&servadd, '\0', sizeof(servadd));
    memcpy(&servadd.sin_addr, hp->h_addr, hp->h_length);
    servadd.sin_port = htons(port);
    servadd.sin_family = AF_INET;

    sprintf(portString, "%d", port);

    while (1){
        if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)                      // Get a socket
            perror_exit("socket");

        pthread_cond_signal(&cond_nonfull);
        if(connect(sock, (struct sockaddr*) &servadd, sizeof(servadd)) != 0)    // Server loops on accept
            perror_exit("connect");

        url = obtain(&pool);                                                    // Pop a URL from the pool

        path = (char *) malloc(strlen(url) + 1);

        strcpy(path, url);

        prepareReq(domain, port, path, sock);                                   // Create and write request

        if((bytesReadFirst = customRead(sock, dummyBuffer, 1500)) < 0)          // Read buffer + some data arbitrarily
            perror_exit("Reading request");

        headerSize  = getHeaderSize(dummyBuffer);                               // Get header size

        contLength  = getContentLength(dummyBuffer);                            // Get content length from header

        bytesRead   = bytesReadFirst - headerSize;                              // Body bytes read

        bytesToRead = contLength - bytesRead;                                   // Body bytes remaining

        dummyFile = (char *) malloc((sizeof(char) * contLength) + 1);

        bcopy(dummyBuffer + headerSize, dummyFile, bytesRead);                  // Copy remaining to temp buffer

        bytesWrote = customRead(sock, dummyFile + bytesRead, bytesToRead);      // Read and save the rest of the contents

        file = createSaveFile(path, saveDir);

        if((fprintf(file, "%s", dummyFile) < 0))                                // Hard save the text
            perror_exit("fprintf");

        fclose(file);                                                           // Close file pointer

        searchUrl(dummyFile, &pool);
        close(sock);

        pthread_mutex_lock(&statistics);                                        // Use mutexes

        bytes += bytesWrote;                                                    // Update stats
        pages++;

        pthread_mutex_unlock(&statistics);                                      // Release lock
    }
}

void perror_exit(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Read from socket and make sure we don't miss chunks
int customRead(int fd, char *payload, int remaining){
    int received = 0, b = 0;

    while (remaining > 0){
        b = read(fd, payload + received, remaining);    // Advance pointer by bytes read
        if(b < 0)
            perror_exit("customRead");
        received += b;
        remaining -= b;
    }

    return b;
}

// Write to socket taking care of packet loss
int customWrite(int fd, char *payload, int remaining) {
    int sent = 0, b = 0;

    while (remaining > 0){
        b = write(fd, payload + sent, remaining);       // Advance pointer by sent bytes
        if(b < 0)
            perror_exit("customWrite");
        sent += b;
        remaining -= b;
    }

    return b;
}

char *getRunTime(char *runTime, time_t start){
    int min, sec, hr, n, ms;
    time_t end;
    double dif;
    time (&end);
    dif = difftime (end,start);
    n = floor(dif);

    if(dif>3600){                               // If the server has been running for over an hour
        min = n/60;
        sec = n%60;
        hr = min/60;
        min = min%60;
        ms = sec%60;
        sprintf(runTime, "%.2d:%.2d:%.2d.%.2d", hr, min, sec, ms);
        return runTime;
    }
    else{                                       // If the server has been running for under an hour
        min = n/60;
        sec = n%60;
        ms = sec%60;
        sprintf(runTime, "%.2d:%.2d.%.2d", min, sec, ms);
        return runTime;
    }
}

int prepareReq(char *domain, int port, char *path, int sockfd){
    char request[500];

    sprintf(request, "GET %s HTTP/1.1\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\nHost: %s\nAccept-Language: en-us\nAccept-Encoding: gzip, deflate\nConnection: Keep-Alive\n\n", path, domain);
    customWrite(sockfd, request, strlen(request));

    return (int) strlen(request);
}

int getContentLength(char *buffer){
    int contLength;
    char *pos;

    pos = strstr(buffer, "Content-Length: ");
    pos = pos + 16;
    contLength = atoi(pos);

    return contLength;
}

int getHeaderSize(char *header){
    char *size;
    int headerSize = 0, start, end;

    start = strlen(header);
    size = strstr(header, "<!DOCTYPE html>");
    end = strlen(size);

    headerSize = start - end;

    return headerSize;
}

FILE *createSaveFile(char *path, char *save_dir){
    char folderName[200], saveDir[200], *folder, tempPath[200];
    FILE *file;
    struct stat st = {0};

    strcpy(saveDir, "./");
    strcat(saveDir, save_dir);
    strcat(saveDir, "/");

    if (stat(saveDir, &st) == -1) {
        if(mkdir(saveDir, 0700) == -1)                  // Create parent dir
            perror("parent mkdir");
    }
    strcpy(tempPath,path);
    folder = strtok(tempPath, "/");

    strcat(saveDir, folder);

    if (stat(saveDir, &st) == -1) {
        if(mkdir(saveDir, 0700) == -1)                  // Create siteX dir
            perror("subdir mkdir");
    }

    strcpy(folderName, save_dir);
    strcat(folderName, path);

    file = fopen(folderName, "w+");                     // Create desired file

    if(file == NULL){
        perror_exit("fopen");
    }

    return file;
}

char *searchUrl(char *inputFile, pool_t *pool){
    char *ahref, *relativeUrl, *temp, *saveptr, *fullpath, *finalUrl = NULL;
    int exists = 0;

    temp = (char *) malloc(strlen(inputFile) + 1);

    while ((ahref = strstr(inputFile, "<a href="))) {
        strcpy(temp, ahref);
        relativeUrl = strtok_r(temp + 8, "\"", &saveptr);                                       // Thread safe strtok

        finalUrl = (char *) malloc(strlen(relativeUrl) + 1);
        fullpath = (char *) malloc(strlen("./save_dir") + strlen(relativeUrl) + 1);

        finalUrl = stringReplace(relativeUrl, finalUrl);

        strcpy(fullpath, "save_dir");
        strcat(fullpath, finalUrl);                                                             // Pass relative path to access

        if ((searchPool(pool, finalUrl) == 1) && ((!fileExist(fullpath)))) {   // Url doesnt exist anywhere
            place(pool, finalUrl);
        }
        if(inputFile != NULL)
            inputFile = ahref + 8;                                                              // Move pointer after a href
    }
    free(temp);
    free(finalUrl);
}

int searchPool(pool_t *pool, char *url){
    int max, i;
    max = pool->count;
    for (i = 0; i <= max; i++) {
        if (!strcmp(url, pool->url[i]))                             // Url exists in pool
            return 0;
    }

    return 1;                                                       // Url doesnt exist
}

char *stringReplace(char *string, char *output){

    if((strstr(string, "..")) != NULL){                             // If the file is like "../siteX/"
        string += 2;
    }
    else if(strstr(string, "./") != NULL)                           // If the file is like "./siteY/"
        string++;

    strcpy(output, string);

    return output;

}

void insertDocfile(char *path){
    FILE *file;

    file = fopen("docfile", "a");                          // Create desired file

    if(file == NULL){
        perror_exit("fopen");
    }

    fprintf(file, "%s\n", path);

    fclose(file);

}

void readSaveFolder(char *path){ // Mr. Stamatopoulos' class notes
    DIR *dp;
    struct dirent *dir;
    char *newName;
    if((dp = opendir(path)) == NULL){                       // Open directory
        perror("save_dir opendir");
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
            strcat(newName, "/");
            insertDocfile(newName);
//            strcat(newName, dir->d_name);
//            if(!strstr(newName, ".html"))                   // Quick and smart way to determine if it's a file
//                strcat(newName, "/");
//
//            if(strstr(newName, ".html"))                    // Populate docfile for search commands
//                insertDocfile(newName);
//
//            if(!strstr(newName, ".html"))
//                readSaveFolder(newName);                        // Find subfolders recursively
        }
        free(newName);
    }
    closedir(dp);                                           // Close directory
}

int dirExists(char *dir){
    struct stat st = {0};

    if (stat(dir, &st) == -1) {                             // Does not exist
        return -1;
    }
    else
        return 0;                                           // Exists
}

int fileExist (char *filename) {
    struct stat   buffer;

    return (stat (filename, &buffer) == 0);
}