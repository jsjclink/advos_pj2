#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    key_t server_msgq_key;
    int server_msgq_id;
    struct msg_buffer_server msg_server;

    struct msg_buffer_client msg_client;
    int shm_id;
    void* shm_ptr;

    /* init */
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

        if((shm_id = shmget(msg_client.shm_key, msg_client.shm_size, 0644)) == -1) {
            perror("shmget error");
            exit(1);
        }
        if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
            perror("shmat error");
            exit(1);
        }

        printf("server: Read from shared memory: \"%s\"\n", (char *)shm_ptr);

        msg_server.msg_type = 1;
        if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
            perror("msgsnd error");
            exit(1);
        }
    }

}