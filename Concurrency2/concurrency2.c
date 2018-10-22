/*
 * Concurrency 1
 * - Anton Synytsia
 * - David Jansen
 * - Eytan Brodsky
 *
 * This program never ends. Use CTRL-C to terminate.
 *
 * Sources:
 * - checking for availability of rdrand instruction was obtained from rdrand_test.c
 * - genrand_int32 and its content was obtained from mt19937ar.c
 * - rdrand: https://codereview.stackexchange.com/questions/147656/checking-if-cpu-supports-rdrand
 * - http://man7.org/linux/man-pages/man3/ was used to reference other functions
 */

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <x86intrin.h>
#include <semaphore.h>


// Global variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int num_forks = 5;
bool use_rdrand;
const char* PHIL_NAMES[] = {
    "Ben Brewster",
    "Jordan B. Peterson",
    "Amir Nayyeri",
    "Fuxin Li",
    "Bechir Hamdaoui"
};


// -----------------------------------------------------------------------------
// mt19937ar.c Stuff
// -----------------------------------------------------------------------------

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] =
            (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }

    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}


// -----------------------------------------------------------------------------
// rdrand Stuff
// -----------------------------------------------------------------------------

bool rdrand_supported() {
    unsigned int eax, ebx, ecx, edx;
    eax = 0x01;

    __asm__ __volatile__(
                         "cpuid;"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(eax)
                         );

    return (ecx & 0x40000000);
}

unsigned int rdrand_uint32() {
    // https://codereview.stackexchange.com/questions/147656/checking-if-cpu-supports-rdrand
    unsigned int value;
    while (_rdrand32_step(&value) == 0);
    return value;
}


// -----------------------------------------------------------------------------
// General random generator
// -----------------------------------------------------------------------------

unsigned int our_rand_uint(unsigned int min, unsigned int max) {
    if (use_rdrand) {
        return min + rdrand_uint32() % (1 + max - min);
    }
    else {
        return min + genrand_int32() % (1 + max - min);
    }
}


// -----------------------------------------------------------------------------
// Producer / Consumer
// -----------------------------------------------------------------------------

void* philosopher(const char* name) {
    while (1) {
        // Acquire mutex
        pthread_mutex_lock(&mutex);
        if (num_forks < 2) {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        num_forks -= 2;
        fprintf(stdout, "Philosopher %s got the forks.\n", name);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
        sleep(our_rand_uint(2, 9));
        pthread_mutex_lock(&mutex);
        num_forks += 2;
        fprintf(stdout, "Philosopher %s released the forks.\n", name);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
        sleep(our_rand_uint(1, 20));
    }

    return 0;
}


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Declare variables
    pthread_t th[5];

    // Check if rdrand is supported
    use_rdrand = rdrand_supported();

    if (use_rdrand)
        fprintf(stdout, "Using rdrand\n");
    else
        fprintf(stdout, "Using mt19937\n");

    // Create consumer and producer threads

    unsigned int i;
    for (i = 0; i < 5; ++i)
        pthread_create(th + i, NULL, philosopher, (void*)(PHIL_NAMES[i]));

    // Join
    for (i = 0; i < 5; ++i)
        pthread_join(th[i], NULL);

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    // Return success
    return 0;
}
