#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snappy.h"

int main() {
    key_t server_msgq_key;
    int server_msgq_id;
    struct msg_buffer_server msg_server;

    struct msg_buffer_client msg_client;
    int shm_id;
    void* shm_ptr;
    struct snappy_env* se;
    size_t compressed_s = 0;

    char *output;
    /* init */
    //init snappy env

    if(!(se = malloc(sizeof(struct snappy_env)))){
        perror("snappy env malloc error");
        exit(1);
    }

    if(snappy_init_env(se) < 0){
        perror("snappy_init_env error\n");
        return -1;
    }
    // get server msgq key
    if((server_msgq_key = ftok(TINY_FILE_KEY, 's')) == -1) {
        perror("ftok error");
        exit(1);
    }
    // get server msgq
    if((server_msgq_id = msgget(server_msgq_key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget server error");
        exit(1);
    }

    /* main */
    while(1) {
        // receive msg from client
        if(msgrcv(server_msgq_id, &msg_client, sizeof(msg_client), 1, 0) == -1) {
            perror("msgrcv error");
            exit(1);
        }

        printf("server: message received\n");

        if((shm_id = shmget(msg_client.shm_key, msg_client.shm_size, 0666)) == -1) {
            perror("shmget error");
            exit(1);
        }
        if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
            perror("shmat error");
            exit(1);
        }
        if(!(output = malloc(snappy_max_compressed_length(msg_client.shm_size)))){
            perror("snappy-c output buffer malloc error");
            exit(1);
        }
        //printf("server: shared memory size: %d, output size: %d\n", msg_client.shm_size,snappy_max_compressed_length(msg_client.shm_size) );
        if(snappy_compress(se, shm_ptr, msg_client.shm_size,output,&compressed_s)< 0){
            perror("snappy-c compress error");
            exit(1);
        }

        //printf("server: snappy-c compressed: %d\n", compressed_s);
        memset(shm_ptr,0,msg_client.shm_size);
        memcpy(shm_ptr,output,compressed_s);
        printf("server: moved to shared memory: \"%s\"\n", (char *)shm_ptr);
        msg_server.msg_type = 1;
        msg_server.comp_size = compressed_s;
        if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
            perror("msgsnd error");
            exit(1);
        }
    }

}