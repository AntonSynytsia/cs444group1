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

typedef struct Node Node;


struct Node{
    unsigned int value;
    Node* next;
};

pthread_mutex_t insertion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t deletion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t search_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t size_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;

Node* head = NULL;
unsigned int list_size = 0;
int flag = 0; //0 means delete isnt working, 1 means it is working.

//always inserts at the end of the list.
void insert(){
    Node* current = head;
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->value = our_rand_uint(0, 10);
    new_node->next = NULL;
    //traverse to the end
    while(current->next != NULL){
        current = current->next;
    }
    current->next = new_node;
    list_size += 1;
}

//always deletes from anywhere in the list
void delete(){
    Node* current = head;
    unsigned int rand_delete_node_index = our_rand_uint(0, list_size - 1);
    //traverse until random node
    unsigned int cur_index = 0;
    while(cur_index != rand_delete_node_index){
        current = current->next;
        cur_index+=1;
    }
    //set last element to null
    current = NULL;
    list_size -= 1;
}
//traverses whole list
void search(){
    /*Node* current = head;
    while(current->next != NULL){
        fprintf(stdout, "Reading %d\n", current->value);
        fflush(stdout);
        current = current->next;
    }*/
    fprintf(stdout, "READ\n");
    fflush(stdout);
}

void* inserter(){
    while(true){
        //Acquire mutex
        pthread_mutex_lock(&insertion_mutex);
        pthread_mutex_lock(&size_mutex);
        fprintf(stdout, "Locking insertion for inserter %02x\n", (unsigned)pthread_self());
        fflush(stdout);
        insert();
        fprintf(stdout, "Unlocking insertion for inserter %02x\n", (unsigned)pthread_self());
        fflush(stdout);
        pthread_mutex_unlock(&size_mutex);
        pthread_mutex_unlock(&insertion_mutex);
        sleep(our_rand_uint(1, 5)); //inserter sleeps from 1-5 seconds
    }
}

void* deleter(){
    while(true){
        //Acquire search and insert mutex
        pthread_mutex_lock(&insertion_mutex);
        pthread_mutex_lock(&search_mutex);
        pthread_mutex_lock(&size_mutex);
        pthread_mutex_lock(&flag_mutex);
        flag=1;
        fprintf(stdout, "Locking deletion for deleter %02x\n",(unsigned)pthread_self());
        fflush(stdout);
        delete();
        fprintf(stdout, "Unlocking deletion for deleter %02x\n", (unsigned)pthread_self());
        fflush(stdout);
        flag = 0;
        pthread_mutex_unlock(&flag_mutex);
        pthread_mutex_unlock(&size_mutex);
        pthread_mutex_unlock(&insertion_mutex);
        pthread_mutex_unlock(&search_mutex);
        sleep(our_rand_uint(1,5)); //deleter sleeps from 1-5 seconds
    }
}

void* searcher(){
    while(true){
        if(flag == 0){
            fprintf(stdout, "Searcher %02x is searching...\n", (unsigned)pthread_self());
            fflush(stdout);
            search();
            fprintf(stdout, "Searcher %02x is done searching\n", (unsigned)pthread_self());
            fflush(stdout);
            sleep(our_rand_uint(1,2));
        }
    }
}

int main(){
    //initialize first two elements
    head = (Node*)malloc(sizeof(Node));
    /*head->value = our_rand_uint(1, 20);
    head->next = (Node*)malloc(sizeof(Node));
    head->next->value = our_rand_uint(1, 20);
    head->next->next = NULL;
    */
    Node* current = head;
    unsigned int j;
    for(j = 0; j < 10; j++){
        current->value = our_rand_uint(1,50);
        current->next = (Node*)malloc(sizeof(Node));
        current = current->next;
    }
    /*current = head;
    for(j = 0; j < 100; j++){
        fprintf(stdout, "%d\n", current->value);
        fflush(stdout);
        current = current->next;
    }*/
    // Declare variables
    pthread_t th[9];

    // Check if rdrand is supported
    use_rdrand = rdrand_supported();

    if (use_rdrand)
        fprintf(stdout, "Using rdrand\n");
    else
        fprintf(stdout, "Using mt19937\n");

    // Create consumer and producer threads

    unsigned int i;
    for (i = 0; i < 3; ++i)
        pthread_create(th + i, NULL, inserter, NULL);
    for(; i < 6; ++i)
        pthread_create(th + i, NULL, deleter, NULL);
    for(; i < 9; ++i)
        pthread_create(th + i, NULL, searcher, NULL);

    // Join
    for (i = 0; i < 9; ++i)
        pthread_join(th[i], NULL);

    // Destroy mutex
    pthread_mutex_destroy(&insertion_mutex);
    pthread_mutex_destroy(&deletion_mutex);
    pthread_mutex_destroy(&search_mutex);
    pthread_mutex_destroy(&size_mutex);
    
    //TODO free list
    // Return success
    return 0;
}
