CC=gcc

all: myhttpd
	(cd Crawler && $(MAKE))
	
myhttpd: main.o functions.o
	$(CC) -o myhttpd main.o functions.o -lpthread -lm

main.o:
	$(CC) -c main.c

functions.o:
	$(CC) -c functions.c

subsystem:
	cd Crawler && $(MAKE)

clean:
		rm -f ./main.o ./functions.o ./myhttpd