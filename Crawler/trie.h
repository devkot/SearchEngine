// forward declaration of all necessary structs
typedef struct TrieNode TrieNode;
typedef struct PostingListNode PostingListNode;
typedef struct TrieNodeList TrieNodeList;

// The list containing all the documents a word is in, along with the number of appearances in each document
// This is a LIFO list, to optimize insertion time during Trie initialization
// It also contains a pointer to the document node, instead of its id, to optimize query search times
struct PostingListNode {
    char *fileName;         // the name of the file
    int lineNumber; 		// the line of the document the word was found inside
    int n;                  // number of appearances
    PostingListNode *next;  // pointer to next node (if any)
};

// Ordered list (by letter) that contains all the pointers of a trie node to all the next available nodes (letters)
struct TrieNodeList{
	char letter;            // The letter the included trie node represents
	TrieNode *node;
	TrieNodeList *next;
};

// The Trie data structure. Contains all words found in docfile
struct TrieNode {
    int frequency;                // the total frequency of a word (if frequency > 0, this is a leaf node)
    int nDocs;                    // the number of different unique docs the word is found
    TrieNodeList *charList;       // list of pointers to all the possible next letters of a word
    PostingListNode *postingList; // pointer to the head of the posting list of the word (if this node is a leaf)
};

TrieNode * createTrieNode();
TrieNode * insertWord(TrieNode *, char *, int, char *);
TrieNode * findNextTrieNode(TrieNode *, char, int);
TrieNode * searchTrie(TrieNode *, char *);
PostingListNode * getPostingList(TrieNode *root, char *word);
void destroyTrie(TrieNode *);

PostingListNode * createPostingList(int, char *);
PostingListNode * addToPostingList(PostingListNode *, int, int *, char *);
int getDocFrequency(PostingListNode *, char *);
void destroyPostingList(PostingListNode *);

