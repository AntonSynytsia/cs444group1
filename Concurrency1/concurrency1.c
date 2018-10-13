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
 * - rdrand: https://stackoverflow.com/questions/43389380/working-example-intel-rdrand-in-c-language-how-to-generate-a-float-type-number
 * - http://man7.org/linux/man-pages/man3/ was used to reference other functions
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>

#define MAX_JOBS 100

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

typedef struct {
    int m_num;
    int m_wait;
} Job;

// Global variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
Job* jobs;
unsigned int jhead;
unsigned int jtail;
unsigned int jtotal_produced;
unsigned int jtotal_consumed;
int use_rdrand;

// rdrand
int rdrand(int* out, int min, int max) {
    int retries = 10;
    unsigned long long rand64;

    while(retries--) {
        if ( __builtin_ia32_rdrand64_step(&rand64) ) {
            *out = (int)((float)rand64 / ULONG_MAX * (max - min)) + min;
            return 1;
        }
    }

    return 0;
}

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

void* producer(void *tid) {
    while (1) {
        // Acquire mutex
        pthread_mutex_lock(&mutex);
        // Continue if full
        int jnext = jtail + 1;
        if (jnext == MAX_JOBS) jnext = 0;
        if (jnext == jhead) {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        // Create job
        if (use_rdrand) {
            rdrand(&(jobs[jtail].m_num), 0, 1000);
            rdrand(&(jobs[jtail].m_wait), 3, 7);
        }
        else {
            jobs[jtail].m_num = genrand_int32() % 1000;
            jobs[jtail].m_wait = 3 + (genrand_int32() % (8 - 3));
        }
        // Track total jobs
        ++jtotal_produced;
        // Display
        fprintf(stdout, "PRODUCE Job %d: num %d, wait %d\n", jtotal_produced, jobs[jtail].m_num, jobs[jtail].m_wait);
        fflush(stdout);
        // Advance jtail
        jtail = jnext;
        // Release mutex
        pthread_mutex_unlock(&mutex);
        // Wait some time before generating next job for proof of concurrency
        usleep(500000);
    }

    return 0;
}

void* consumer(void *tid) {
    int dt;

    while (1) {
        dt = 0;
        // Acquire mutex
        pthread_mutex_lock(&mutex);
        // Process job if not empty
        if (jhead != jtail) {
            ++jtotal_consumed;
            // Display
            fprintf(stdout, "CONSUME Job %d: num %d, waiting %d seconds before consuming next job...\n", jtotal_consumed, jobs[jhead].m_num, jobs[jhead].m_wait);
            fflush(stdout);
            // Save job time to dt
            dt = jobs[jhead].m_wait;
            // Advance jhead
            ++jhead;
            if (jhead == MAX_JOBS) jhead = 0;
        }
        // Release mutex
        pthread_mutex_unlock(&mutex);
        // Sleep for dt
        if (dt) sleep(dt);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Declare variables
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    pthread_t th1;
    pthread_t th2;

    // Assign global variables
    jobs = (Job*)malloc(sizeof(Job) * MAX_JOBS);
    jhead = 0;
    jtail = 0;
    jtotal_produced = 0;
    jtotal_consumed = 0;

    // Determine if rdrand is supported
    eax = 0x01;

    __asm__ __volatile__(
                         "cpuid;"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(eax)
                         );

    if (ecx & 0x40000000) {
        // use rdrand
        use_rdrand = 1;
        fprintf(stdout, "Using rdrand\n");
    }
    else {
        // use mt19937
        use_rdrand = 0;
        fprintf(stdout, "Using mt19937\n");
    }

    // Create consumer and producer threads
    pthread_create(&th1, NULL, consumer, NULL);
    pthread_create(&th2, NULL, producer, NULL);

    // Join
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    // Return success
    return 0;
}
