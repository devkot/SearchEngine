#define POOL_SIZE 50

typedef struct{
    char url[POOL_SIZE][100];
    int start;
    int end;
    int count;
}pool_t;

struct prod_args{
    int port_c;
    int port_s;
    int numThreads;
    char *url;
    char *domain;
    char *saveDir;
};

void initializePool(pool_t *);
void place(pool_t *, char *);
void *producer(void *);
void *consumer(void *);
void perror_exit(char *);
void readSaveFolder(char *);
void insertDocfile(char *);

int customWrite(int , char *, int );
int customRead(int , char *, int );
int getContentLength(char *);
int prepareReq(char *, int, char *, int);
int getHeaderSize(char *);
int searchPool(pool_t *, char *);
int dirExists(char *);
int fileExist (char *);

char *getRunTime(char *, time_t);
char *obtain(pool_t *);
char *searchUrl(char *, pool_t *);
char *stringReplace(char *, char *);

FILE *createSaveFile(char *, char *);



