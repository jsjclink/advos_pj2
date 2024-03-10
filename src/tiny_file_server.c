#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>

struct current_serving_state current_servings[MAX_SERVINGS];
int n_sms;
int sms_size;

int get_serving_idx(struct msg_buffer_client msg_client) {
    int i;
    for(i = 0; i < n_sms; i++) {
        if(current_servings[i].cli_msgqid == msg_client.cli_msgqid){
            break;
        }
    }
    return i;
}

int get_new_serving_idx() {
    int i;
    for(i = 0; i < n_sms; i++) {
        if(current_servings[i].valid == 0){
            break;
        }
    }
    return i;
}

void shm_response(struct msg_buffer_client msg_client) {
    key_t shm_key;
    int shm_id;
    void* shm_ptr;
    struct msg_buffer_server msg_server;

    int idx = get_new_serving_idx();
    if(idx == n_sms){

        msg_server.msg_type = 1;
        msg_server.message_type = REQUEST_REJECT;

        if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
            perror("msgsnd error");
            exit(1);
        }
    }
    
    if((shm_key = ftok("shm_key", idx)) == -1) {
        perror("ftok error");
        exit(1);
    }
    if ((shm_id = shmget(shm_key, msg_client.shm_size, 0644 | IPC_CREAT)) == -1) {
        perror("shmget error");
        exit(1);
    }
    if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
        perror("shmat error");
        exit(1);
    }

    /* set current serving state */
    current_servings[idx].cli_msgqid = msg_client.cli_msgqid;
    current_servings[idx].shm_ptr = shm_ptr;
    current_servings[idx].valid = 1;

    printf("server: generated current serving state. idx: %d\n", idx);

    /* send message to client */
    msg_server.msg_type = 1;
    msg_server.shm_key = shm_key;

    if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }

    printf("server: sent shared memory response\n");
}

void compress_response(struct msg_buffer_client msg_client) {
    struct msg_buffer_server msg_server;

    int idx = get_serving_idx(msg_client);

    printf("server: compressing ... cli_idx: %d\n", idx);

    printf("server: Read from shared memory: \"%s\"\n", (char *)(current_servings[idx].shm_ptr));

    /* send message to client */
    msg_server.msg_type = 1;
    
    if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }

    printf("server: compress completed, sent compress response\n");

    /* invalidate current serving state */
    current_servings[idx].valid = 0;
}

int main(int argc, char *argv[]) {
    key_t server_msgq_key;
    int server_msgq_id;
    struct msg_buffer_client msg_client;

    /* init */
    // get arguments from command line
    n_sms = argv[2];
    sms_size = argv[4];

    // init current_servings
    for(int i = 0; i < n_sms; i++){
        current_servings[i].valid = 0;
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

        switch(msg_client.message_type) {
            case SHM_REQUEST: 
                shm_response(msg_client);
                break;
            case FILE_CONTENT_FILLED: 
                compress_response(msg_client);
                break;
        };

        // if((shm_id = shmget(msg_client.shm_key, msg_client.shm_size, 0644)) == -1) {
        //     perror("shmget error");
        //     exit(1);
        // }
        // if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
        //     perror("shmat error");
        //     exit(1);
        // }

        // printf("server: Read from shared memory: \"%s\"\n", (char *)shm_ptr);

        // msg_server.msg_type = 1;
        // if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        //     perror("msgsnd error");
        //     exit(1);
        // }
    }

}