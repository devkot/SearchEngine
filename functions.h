#define POOL_SIZE 6

typedef struct{
    int fd[POOL_SIZE];
    int start;
    int end;
    int count;
}pool_t;

void initialize(pool_t *);
void place(pool_t *, int);
void *producer(void *);
void *consumer(void *);
void perror_exit(char *);
void *command(void *);

int customWrite(int , char *, int );
int customRead(int , char *, int );
int fileExists(char *);
int obtain(pool_t *);
int hasPermissions(char *);
int getFilesize(char*);

char* getFormattedTime(void);
char *getRunTime(char *, time_t);

