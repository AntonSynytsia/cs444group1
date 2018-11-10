COMPILER = gcc
#CFLAGS = -Wall -fpic -coverage -lm -std=c99
CFLAGS = -Wall -lpthread -mrdrnd

all: concurrency3

concurrency3: concurrency3.c
	$(COMPILER) -o concurrency3 concurrency3.c $(CFLAGS)

clean:
	rm -f concurrency3.exe concurrency3 *.o *.gcov *.gcda
