#include <stdio.h>
#include <pthread.h>

void* t_function(void *arg){
    printf("a: %d\n", *((int*)arg));

    for(int i = 0; i < 100; i++){
        printf("pthread: hi_%d\n", i);
    }
}

int main(){
    pthread_t pthread;
    int status;
    int a = 1;
    int thr_id = pthread_create(&pthread, NULL, t_function, (void *) &a);

    for(int i = 0; i < 100; i++){
        printf("main: hi_%d\n", i);
    }

    pthread_join(pthread, NULL);
}