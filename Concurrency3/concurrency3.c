#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <x86intrin.h>
#include <semaphore.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int num_users = 0;
bool use_rdrand;

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

bool available = true;
void* user(){
    while(true){
        //Acquire mutex
        pthread_mutex_lock(&mutex);
        if(num_users >= 3 || available == false){
            available = false;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        num_users+=1;
        fprintf(stdout, "A user joined!\n");
        fflush(stdout);
        fprintf(stdout, "Number of users at the moment: %d.\n", num_users);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
        unsigned int use_time = our_rand_uint(2, 10);
        fprintf(stdout, "Using resource for %d seconds.\n", use_time);
        fflush(stdout);
        sleep(use_time);
        pthread_mutex_lock(&mutex);
        num_users-=1;
        if(num_users == 0)
            available = true;
        fprintf(stdout, "User is done with resource.\n");
        fflush(stdout);
        fprintf(stdout, "Number of users at the moment: %d.\n", num_users);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    pthread_t th[5];

    //Check if rdrand is supported
    use_rdrand = rdrand_supported();

    if(use_rdrand)
        fprintf(stdout, "Using rdrand\n");
    else
        fprintf(stdout, "Using mt19937\n");

    //create user thread

    unsigned int i;
    for(i = 0; i < 10; i++){
        pthread_create(th + i, NULL, user, NULL);
    }

    //Join threads
    for(i = 0; i < 5; i++){
        pthread_join(th[i], NULL);
    }
    //Destroy mutex
    pthread_mutex_destroy(&mutex);
    return 0;
}


