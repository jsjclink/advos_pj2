#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    key_t server_msgq_key;
    int server_msgq_id, client_msgq_id;
    struct msg_buffer_server msg_server;
    struct msg_buffer_client msg_client;
    key_t shm_key;
    int shm_id;
    void* shm_ptr;

    int fd;
    struct stat file_stat;

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

    // create client msgq
    if((client_msgq_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1) {
        perror("msgget client error");
    }

    printf("client: init completed\n");

    /* open file */
    if ((fd = open("tmp_file.txt", O_RDWR)) == -1) {
        perror("open failed");
        exit(1);
    }

    if (fstat(fd, &file_stat) == -1) {
        perror("fstat failed");
        close(fd);
        exit(1);
    }

    printf("client: file open completed\n");

    /* request shared memory to server */
    msg_client.msg_type = 1;
    msg_client.cli_msgqid = client_msgq_id;
    msg_client.message_type = SHM_REQUEST;
    msg_client.shm_size = file_stat.st_size;

    if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }

    printf("client: requested shared memory to server\n");

    /* wait for shared memory response */
    printf("client: waiting for shared memory response\n");

    if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
        perror("msgrcv error");
        exit(1);
    }

    if(msg_server.message_type == REQUEST_REJECT) {
        printf("client: request rejected. abort\n");
        exit(1);
    }

    printf("client: received shared memory response\n");

    /* copy file content to shared memory */
    if((shm_id = shmget(msg_server.shm_key, msg_client.shm_size, 0644)) == -1) {
        perror("shmget error");
        exit(1);
    }
    if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
        perror("shmat error");
        exit(1);
    }
    // write file content to shared memory
    if (read(fd, shm_ptr, file_stat.st_size) == -1) {
        perror("read failed");
        shmdt(shm_ptr);
        close(fd);
        exit(1);
    }

    printf("client: file content copy completed\n");

    /* send file_content_filled alert to server */
    msg_client.msg_type = 1;
    msg_client.cli_msgqid = client_msgq_id;
    msg_client.message_type = FILE_CONTENT_FILLED;

    if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }
    
    /* wait for response */
    printf("client: waiting for file compress response\n");

    if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
        perror("msgrcv error");
        exit(1);
    }

    printf("client: received file compress response\n");


    /* shared memory */
    // if((shm_key = ftok("shm_key", 'a')) == -1) {
    //     perror("ftok error");
    //     exit(1);
    // }

    // if ((shm_id = shmget(shm_key, file_stat.st_size, 0644 | IPC_CREAT)) == -1) {
    //     perror("shmget error");
    //     exit(1);
    // }

    // if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
    //     perror("shmat error");
    //     exit(1);
    // }

    // write file content to shared memory
    // if (read(fd, shm_ptr, file_stat.st_size) == -1) {
    //     perror("read failed");
    //     shmdt(shm_ptr);
    //     close(fd);
    //     exit(1);
    // }

    /* send message */
    // msg_client.msg_type = 1;
    // msg_client.cli_msgqid = client_msgq_id;
    // msg_client.shm_key = shm_key;
    // msg_client.shm_size = file_stat.st_size;

    // if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
    //     perror("msgsnd error");
    //     exit(1);
    // }

    // /* wait for response */
    // if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
    //     perror("msgrcv error");
    //     exit(1);
    // }
    // printf("message received");

    // shared memory, file close
    shmdt(shm_ptr);
    close(fd);
}