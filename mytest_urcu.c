#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <urcu/tls-compat.h>
#include <urcu.h>

//setting of verbose_mode;
int verbose_mode = 0;

//the number of readers and writers
int readers = 0,writers = 0;

//control the pthread's status
static volatile int go = 0;
int duration = 0;

//the test data
static int *test_rcu_pointer;


//TLS varible
static DEFINE_URCU_TLS(unsigned long long, nr_writes);
static DEFINE_URCU_TLS(unsigned long long, nr_reads);

//?????
static unsigned long long __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *tot_nr_writes;
static unsigned long long __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *tot_nr_reads;

void *reader(void *data)
{
    unsigned long tidx = (unsigned long)data;
    int *local_ptr;
    
    if(verbose_mode)
        printf("thread_begin %s, tid %lu\n","reader", pthread_self());

    rcu_register_thread();
    
    while(!go){}
        
    for(;;){
        rcu_read_lock();
        local_ptr = rcu_dereference(test_rcu_pointer);
        if(local_ptr)
            assert(*local_ptr == 8);
        rcu_read_unlock();
        URCU_TLS(nr_reads)++;
        if(go==0)
            break;
    }

    rcu_unregister_thread();

    tot_nr_reads[tidx] = URCU_TLS(nr_reads);
    return ((void*)1);
}
void *writer(void *data)
{
    unsigned long tidx = (unsigned long)data;
    int *new, *old;

    if(verbose_mode)
        printf("thread_begin %s, tid %lu\n","writer", pthread_self());
 
    while(!go){};
    
    for(;;){
        new = malloc(sizeof(int));
        assert(new);
        *new = 8;
        //要知道这里究竟发生了什么？原子操作还是MB的使用??
        old = rcu_xchg_pointer(&test_rcu_pointer,new);
        synchronize_rcu();
        free(old);
        
        URCU_TLS(nr_writes)++;
        if(go==0)
            break;    
    }

    tot_nr_writes[tidx] = URCU_TLS(nr_writes);
    return ((void*)2);
}

int main(int argc, char **argv)
{
    int i = 0;
    int err;  
    //declare pthread for writer and reader
    pthread_t *tid_reader, *tid_writer;

    //the total of reads and writes
    unsigned long long tot_reads = 0, tot_writes = 0;

    err = sscanf(argv[1], "%u", &readers);
    if(err != 1){
        return -1;    
    }

    err = sscanf(argv[2], "%u", &writers);
    if(err != 1){
        return -1;
    }

    err = sscanf(argv[3], "%u", &duration);
    if(err != 1){
        return -1;    
    }

    if(argc > 4){
        if(argv[4][1] == 'v')
            verbose_mode = 1;    
    }
    //allow memory for varibales
    tid_reader = calloc(readers, sizeof(*tid_reader));
    tid_writer = calloc(writers, sizeof(*tid_writer));
    tot_nr_reads = calloc(readers, sizeof(*tot_nr_reads));
    tot_nr_writes = calloc(writers, sizeof(*tot_nr_writes));

    for(i = 0; i < readers; i++){
        err = pthread_create(&tid_reader[i], NULL, reader, (void *)(long)i);
        if(err != 0){
            exit(1);
        }
    }

    for(i = 0; i < writers; i++){
        err = pthread_create(&tid_writer[i], NULL, writer, (void *)(long)i);
        if(err != 0)
            exit(1);    
    }
    
    //thread go!
    go = 1;

    sleep(duration);

    //thread stop!
    go = 0;

    for(i = 0; i < readers; i++){
        err = pthread_join(tid_reader[i], NULL);
        if(err != 0)
            exit(1);
        tot_reads += tot_nr_reads[i];
    }

    for(i = 0; i < writers; i++){
        err = pthread_join(tid_writer[i], NULL);
        if(err != 0)
            exit(1);
        tot_writes += tot_nr_writes[i];    
    }

    printf("%-25s, test duration: %4lu, readers: %3u, writers: %3u,total reads: %12llu,total writes: %12llu \n",argv[0],duration,readers,writers,tot_reads,tot_writes);
    
    free(test_rcu_pointer);
    free(tid_reader);
    free(tid_writer);
    free(tot_nr_reads);
    free(tot_nr_writes);

    return 0;
}
