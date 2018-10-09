COMPILER = gcc
#CFLAGS = -Wall -fpic -coverage -lm -std=c99
CFLAGS = -Wall -lpthread -mrdrnd

all: concurrency1

concurrency1: concurrency1.c
	$(COMPILER) -o concurrency1 concurrency1.c $(CFLAGS)

clean:
	rm -f concurrency1.exe concurrency1 *.o *.gcov *.gcda
