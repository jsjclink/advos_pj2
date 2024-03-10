#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    /* shared memory */
    if((shm_key = ftok("shm_key", 'a')) == -1) {
        perror("ftok error");
        exit(1);
    }

    if ((shm_id = shmget(shm_key, file_stat.st_size, 0666 | IPC_CREAT)) == -1) {
        perror("shmget error");
        exit(1);
    }

    if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
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

    /* send message */
     msg_client.msg_type = 1;
    msg_client.cli_msgqid = client_msgq_id;
    msg_client.shm_key = shm_key;
    msg_client.shm_size = file_stat.st_size;

    if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }

    /* wait for response */
    if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
        perror("msgrcv error");
        exit(1);
    }

    printf("message received\n");
    printf("client: Read from shared memory: \"%c\"\n", (char *)(shm_ptr +4));
    // shared memory, file close
    shmdt(shm_ptr);
    close(fd);
}