CC=gcc

all: mycrawler worker

mycrawler: crawler.o utilities.o support.o jobexec.o
	$(CC) -o mycrawler crawler.o utilities.o support.o jobexec.c -lpthread -lm

worker: worker.o functions.o trie.o helpers.o
	$(CC) -o worker worker.o functions.o trie.o helpers.o

worker.o:
	$(CC) -c worker.c

jobexec.o:
	$(CC) -c jobexec.c

crawler.o:
	$(CC) -c crawler.c

support.o:
	$(CC) -c support.c

functions.o:
	$(CC) -c functions.c

trie.o:
	$(CC) -c trie.c

helpers.o:
	$(CC) -c helpers.c

utilities.o:
	$(CC) -c utilities.c

clean:
		rm -f ./crawler.o ./functions.o ./trie.o ./helpers.o ./worker.o ./mycrawler ./utilities.o ./worker ./support.o ./jobexec.o