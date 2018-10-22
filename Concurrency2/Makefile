COMPILER = gcc
#CFLAGS = -Wall -fpic -coverage -lm -std=c99
CFLAGS = -Wall -lpthread -mrdrnd

all: concurrency2

concurrency2: concurrency2.c
	$(COMPILER) -o concurrency2 concurrency2.c $(CFLAGS)

clean:
	rm -f concurrency2.exe concurrency2 *.o *.gcov *.gcda
