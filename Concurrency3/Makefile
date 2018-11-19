COMPILER = gcc
#CFLAGS = -Wall -fpic -coverage -lm -std=c99
CFLAGS = -Wall -lpthread -mrdrnd

all: concurrency3 concurrency3_2

concurrency3: concurrency3.c
	$(COMPILER) -o concurrency3 concurrency3.c $(CFLAGS)

concurrency3_2: concurrency3_2.c
	$(COMPILER) -o concurrency3_2 concurrency3_2.c $(CFLAGS)

clean:
	rm -f concurrency3.exe concurrency3 *.o *.gcov *.gcda
	rm -f concurrency3_2.exe concurrency3_2 *.o *.gcov *.gcda
