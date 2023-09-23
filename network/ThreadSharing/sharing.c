//
// Created by 杨君鹏 on 2023/9/19.
//

#include "../../include/csapp.h"
#include <stdio.h>

#define N 2

void *thread(void *vargp);

char **ptr;     /* Global variable */

int main(){
    int i ;
    pthread_t tid;
    char *msgs[N] = {
            "Hello from foo",
            "Hello from bar"
    };

    ptr = msgs;
    for (int i = 0; i < N; ++i) {
        pthread_create(&tid,NULL,thread,(void *)i);
    }
    pthread_exit(NULL);
}


void *thread(void *vargp){
    int myid = (int)vargp;
    static int cnt = 0;
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid],++cnt);
    return NULL;
}