typedef struct TrieNode TrieNode;
typedef struct DocumentNode DocumentNode;
typedef struct QueryDocScoreNode QueryDocScoreNode;
typedef struct QueryWordInfoNode QueryWordInfoNode;
typedef struct UniqueDocNode UniqueDocNode;

int searchQuery(char *, TrieNode *, int, int);
int countWords(char *);
int wordCount(int *, int *, int *, int);
int readFile(char *, TrieNode *,int *, int *, int *);

void readFolder(char *, TrieNode *,int *, int *, int *);
void writeBack(int, char *);
//void alarmHandler(int);

char * freqCount(char *, TrieNode *, char *, int);