#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <unistd.h> 
#include <string.h> 

static pthread_mutex_t mutex;
static pthread_cond_t cond;
static int global_shared_resource = 0;

static void* consumer_thread(void *arg)
{
    for (;;) {
        // mutex lock
        pthread_mutex_lock(&mutex);

        while (global_shared_resource <= 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        // consume
        while (global_shared_resource)
        global_shared_resource--;

        // condition siganl
        pthread_cond_signal(&cond);
    }
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int ret;

    // initialize mutex lock and condition variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // create new thread
    ret = pthread_create(&tid, NULL, consumer_thread, NULL);
    if (ret) {
        fprintf(stderr, "pthread create error:%s\n",strerror(ret));
        exit(-1);
    }

    // producer
    while (1) {
        // mutex lock
        pthread_mutex_lock(&mutex);
        // product
        global_shared_resource++;
        // mutex unlock
        pthread_mutex_unlock(&mutex);
        // condition siganl
        pthread_cond_signal(&cond);
    }

    exit(0);
}