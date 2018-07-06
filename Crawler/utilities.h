typedef struct DocumentNode DocumentNode;
typedef struct TrieNode TrieNode;

// The data structure that stores all the documents
struct DocumentNode {
    char *text;     // the actual text of the document
    int nWords;     // the number of words of the document
};

void addDocument(DocumentNode *, int, char *, int);
void destroyDocuments(DocumentNode *, int);
void distributeDocs(DocumentNode *, int *, int, int *);
void printMenu(int);
void writeQuery(char *, char *, int, int *, int *, int);
void wipeBuffer(char *);

int determineWorkload(int, int, int *);
int * splitWorkload(int , int *, int);
int initialize(FILE *, DocumentNode *);
int countWords(char *);