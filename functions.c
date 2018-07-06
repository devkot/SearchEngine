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

#include "functions.h"

extern int bytes, pages;

extern pthread_mutex_t mtx, statistics;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonfull;
extern pool_t pool;
time_t start;

void initialize(pool_t *pool) {
    pool->start = 0;
    pool->end   = -1;
    pool->count = 0;
}

// Place fd in the pool
void place(pool_t *pool, int fd) {

    pthread_mutex_lock(&mtx);

    while (pool->count >= POOL_SIZE){
        pthread_cond_wait(&cond_nonfull, &mtx);
    }

    pool->end = (pool->end + 1) % POOL_SIZE;
    pool->fd[pool->end] = fd;
    pool->count++;

    pthread_mutex_unlock(&mtx);
}

// Get fd from the pool
int obtain(pool_t *pool) {
    int fd = 0;

    pthread_mutex_lock(&mtx);

    while (pool->count <= 0){
        pthread_cond_wait(&cond_nonempty, &mtx);
    }
    fd = pool->fd[pool->start];
    pool->start = (pool->start + 1) % POOL_SIZE;
    pool->count--;
    pthread_mutex_unlock(&mtx);

    return fd;
}

// Thread responsible for listening requests
void *producer(void *ptr) {
    struct sockaddr_in myaddr;
    int lsock, csock, serving_port;

    serving_port = *((int *) ptr);
    time (&start);

    if((lsock = socket( AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(serving_port);
    myaddr.sin_family = AF_INET;

    // Resolve address already in use
    if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &myaddr, sizeof(myaddr)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if(bind(lsock, (struct sockaddr *)&myaddr, sizeof(myaddr)))
        perror_exit("bind");

    if(listen(lsock, 5) != 0)
        perror_exit("listen");

    printf("Server is ready. Awaiting client connections on port %d\n", serving_port);
    while(1){
        if((csock = accept(lsock, NULL, NULL)) < 0)
            perror_exit("accept");

        place(&pool, csock);
        pthread_cond_signal(&cond_nonempty);
    }
    close(csock);

}

// Get fds from the pool and serve them to the user
void *consumer(void *ptr) {
    char temp[1000], *type, *requestPath, rootDir[100], header[1000], *payload = NULL, *packet = NULL;
    int fd, bytesWrote, socketID, getRequest;

    while (1){
        pthread_cond_signal(&cond_nonfull);

        socketID = obtain(&pool);                                               // Pop a fd from the pool

        printf("Consumer received a request\n");

        if((getRequest = read(socketID, temp, 1000)) < 0)                       // Read request
            perror_exit("Reading request");

        if(getRequest == 0)                                                     // If the crawler has closed
            continue;

        type = strtok(temp, " ");                                               // Save GET request
        requestPath = strtok(NULL, " ");                                        // Get the given file path

        if(strcmp(type, "GET") != 0)                                            // If it isn't a GET request
            continue;

        strcpy(rootDir, "./root_dir");
        strcat(rootDir, requestPath);                                           // Concatenate the path

        if(fileExists(rootDir) == 1){                                           // File doesn't exist 404
            sprintf(header,
                    "HTTP/1.1 404 Not Found\nDate: %s\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n<html>The page you were looking for could not be found.</html>"
                    , getFormattedTime(), 62);
            write(socketID, header, strlen(header));
        }
        else if (hasPermissions(rootDir) == 0) {                                // Forbidden 403
            sprintf(header,
                    "HTTP/1.1 403 Forbidden\nDate: %s\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n<html>These are not the droids you are looking for..or you are not allowed to access this page.</html>",
                    getFormattedTime(), 102);
            write(socketID, header, strlen(header));
        }
        else {                                                                  // OK to send
            sprintf(header,
                    "HTTP/1.1 200 OK\nDate: %s\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n",
                    getFormattedTime(), getFilesize(rootDir));

            if((fd = open(rootDir, O_RDONLY)) < 0)                              // Open the file
                perror_exit("open");

            payload = (char *) malloc(sizeof(char) * getFilesize(rootDir));     // Allocate memory for the text

            if((customRead(fd, payload, getFilesize(rootDir))) < 0)             // Custom read function to read the entire doc
                perror_exit("read");

            packet = (char *) malloc(strlen(header) + strlen(payload) + 1);     // Allocate memory for the header + payload

            strcpy(packet, header);
            strcat(packet, payload);                                            // Header + payload

            printf("Serving document\n");
            bytesWrote = customWrite(socketID, packet, strlen(packet));         // Write to socket and save bytes for STATS

            close(fd);

            pthread_mutex_lock(&statistics);                                    // Use mutexes

            bytes += bytesWrote;                                                // Update stats
            pages++;

            pthread_mutex_unlock(&statistics);                                  // Release lock

        }
        close(socketID);
        free(packet);
        free(payload);
    }
}

void *command(void *ptr){
    char telbuf[20], *command, stats[200], runTime[100];
    struct sockaddr_in myaddr;
    int lsock, csock, command_port;

    command_port = *((int *) ptr);

    if((lsock = socket( AF_INET, SOCK_STREAM, 0)) < 0)
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
        perror_exit("bind");

    if(listen(lsock, 5) != 0)
        perror_exit("listen");

    printf("Waiting for commands on port %d\n", command_port);
    while(1){
        printf("Waiting for clients\n");
        if((csock = accept(lsock, NULL, NULL)) < 0)
            perror_exit("accept");

        bzero(telbuf, strlen(telbuf));
        if((read(csock, telbuf, 20)) < 0)
            perror_exit("Reading request");

        telbuf[20] = '\0';                      // Append terminating character

        command = strtok(telbuf, "\r\n");       // Remove chars appended by telnet

        if (!strcmp(command, "STATS")){
            bzero(stats, 200);
            sprintf(stats, "Server up for %s, served %d pages, %d bytes\n", getRunTime(runTime, start), pages, bytes);
            customWrite(csock, stats, strlen(stats));
        }
        else if (!strcmp(command, "SHUTDOWN")){
            bzero(stats, 200);
            sprintf(stats, "Server shutting down...\n");
            customWrite(csock, stats, 25);
            printf("Received shutdown command.\n");
            close(csock);
            exit(0);
        }
        else {
            bzero(stats, 200);
            sprintf(stats, "Invalid command\n");
            customWrite(csock, stats, 17);
        }

    }
}

void perror_exit(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Check if the file exists for 404
int fileExists(char *filename) {
    struct stat buffer;
//    printf("Searching for file\n");

    if(stat(filename, &buffer) != 0)
        return 1;
    else
        return 0;
}

// Check whether the server has permissions to access file for 403
int hasPermissions(char *filename) {
    struct stat buffer;
//    printf("Checking file permissions\n");

    if(stat(filename, &buffer) != 0)
        perror_exit("stat");

    mode_t bits = buffer.st_mode;
    if(bits & S_IRUSR){
        // User doesn't have read privileges
        return 1;
    }
}

// Return file size or 0 if it doesn't exist
int getFilesize(char *filename) {
    struct stat buffer;

    if(stat(filename, &buffer) != 0) {
        return 0;
    }
    return (int) buffer.st_size;
}

// Get timestamp for the header
char *getFormattedTime(void) {
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    static char timeStamp[200];
    strftime(timeStamp, sizeof(timeStamp), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

    return timeStamp;
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