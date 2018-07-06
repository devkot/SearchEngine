typedef struct DocumentNode DocumentNode;
typedef struct QueryWordInfoNode QueryWordInfoNode;
typedef struct TrieNode TrieNode;
typedef struct WordSearchNode WordSearchNode;

// The data structure that contains a word and its corresponding trie node
struct QueryWordInfoNode{
	char *word;
	TrieNode *wordNode;
};

// The data structure that contains all the necessary information for the search query
struct WordSearchNode {
    char *fileName;
    int line;
    char *contents;
    char *word;
    WordSearchNode *next;
};

void destroyWordSearchNodes(WordSearchNode *);
void writeLog(WordSearchNode *, char *, char *,char *, int, int *, int *, int *);

char * readLine(char *, int);
char * getFormattedTime(void);

int isDuplicate(WordSearchNode *, char *);

WordSearchNode *addWordSearchNode(WordSearchNode *, char *, int , char *, int *, char *);
