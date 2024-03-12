#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/* Calling SYNC TINYFILE 
   Input: file name , result buffer
   Output: Returns 0 if successful
*/
int call_service(char* file_name, char** result_buffer) {
    int server_msgq_id, server_private_msgq_id, client_msgq_id;
    struct msg_buffer_server msg_server;
    struct msg_buffer_client msg_client;

    struct stat file_stat;

    int sms_size, max_req_size;

    key_t shm_key;
    int shm_id;
    void *shm_ptr; 

    time_t start_time, end_time;
    double elapsed_time;

    /* init */
    start_time = time(NULL);
    // get server msgq key
    key_t server_msgq_key;
    if((server_msgq_key = ftok(TINY_FILE_KEY, 255)) == -1) {
        perror("ftok error");
        return -1;
    }
    // get server msgq
    if((server_msgq_id = msgget(server_msgq_key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget server error");
        return -1;
    }

    // create client msgq
    if((client_msgq_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1) {
        perror("msgget client error");
        return -1;
    }
    // empty client msgq
    struct msg_buffer_client msg;
    while(msgrcv(client_msgq_id, &msg, sizeof(msg), 1, IPC_NOWAIT) != -1);


    printf("cli_msgqid: %d\n", client_msgq_id);

    /* open file */
    int fd;
    if ((fd = open(file_name, O_RDWR)) == -1) {
        perror("open failed");
        return -1;
    }

    if (fstat(fd, &file_stat) == -1) {
        perror("fstat failed");
        close(fd);
        return -1;
    }
    char *file_buffer = malloc(file_stat.st_size * sizeof(char));
    if (read(fd, file_buffer, file_stat.st_size) == -1) {
        perror("read failed");
        close(fd);
        return -1;
    }
    close(fd);

    printf("client: opened file\n");

    /* request sms_size to server */
    msg_client.msg_type = 1;
    msg_client.message_type = SMS_SIZE_REQUEST;
    msg_client.cli_msgqid = client_msgq_id;

    if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
        perror("msgsnd error");
        return -1;
    }

    printf("client: sent SMS_SIZE_REQUEST\n");

    /* wait for SMS_SIZE_RESPONSE */
    if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
        perror("msgrcv error");
        return -1;
    }
    if(msg_server.message_type != SMS_SIZE_RESPONSE) {
        printf("received invalid message\n");
        return -1;
    }

    printf("client: received SMS_SIZE_RESPONSE\n");

    // set infos
    sms_size = msg_server.sms_size;
    max_req_size = msg_server.max_req_size;

    printf("client: sms_size: %d, max_req_size: %d\n", sms_size, max_req_size);

    /* start requesting for compression */
    int count = 0;
    int itr = (file_stat.st_size + max_req_size - 1) / max_req_size;
    *result_buffer = malloc(itr * 2 * sms_size * sizeof(char));

    for(int i = 0; i < itr; i++) {
        /* send service request */
        msg_client.msg_type = 1;
        msg_client.message_type = SERVICE_REQUEST;
        msg_client.cli_msgqid = client_msgq_id;

        if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }

        printf("client: sent SERVICE_REQUEST\n");

        /* wait for FILE_CONTENT_FILL_REQUEST message */
        if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
            perror("msgrcv error");
            return -1;
        }
        if(msg_server.message_type != FILE_CONTENT_FILL_REQUEST) {
            printf("received invalid message\n");
            return -1;
        }

        printf("client: received FILE_CONTENT_FILL_REQUEST\n");

        // set infos
        shm_key = msg_server.shm_key;
        server_private_msgq_id = msg_server.server_private_msgqid;
        if((shm_id = shmget(msg_server.shm_key, sms_size, 0644)) == -1) {
            perror("shmget error");
            return -1;
        }
        if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
            perror("shmat error");
            return -1;
        }

        memcpy(shm_ptr, file_buffer + max_req_size * i, max_req_size);

        /* send FILE_CONTENT_FILLED message to server */
        msg_client.msg_type = 1;
        msg_client.message_type = FILE_CONTENT_FILLED;
        msg_client.cli_msgqid = client_msgq_id;
        if (msgsnd(server_private_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }

        printf("client: sent FILE_CONTENT_FILLED\n");

        printf("client: wait for compress done\n");

        /* wait for COMPRESS_DONE message */
        if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
            perror("msgrcv error");
            return -1;
        }
        if(msg_server.message_type != COMPRESS_DONE) {
            printf("received invalid message\n");
            return -1;
        }

        printf("client: received COMPRESS_DONE\n");

        /* copy result to result_buffer */
        memcpy(*result_buffer + count, shm_ptr, msg_server.compressed_size);
        count += msg_server.compressed_size;

        /* send SHM_USE_FINISHED message to server */
        msg_client.msg_type = 1;
        msg_client.message_type = SHM_USE_FINISHED;
        msg_client.cli_msgqid = client_msgq_id;
        if (msgsnd(server_private_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }

        printf("client: sent SHM_USE_FINISHED\n");

        /* detach shm */
        shmdt(shm_ptr);
    }

    end_time = time(NULL);
    elapsed_time = difftime(end_time, start_time);
    printf("Elapsed time: %.10f seconds\n", elapsed_time);
    printf("client: res_buffer: \"%s\"\n", *result_buffer);
    return 0;
}


int call_sync_service(char* file_name, char** result_buffer) {
    return call_service(file_name, result_buffer);
}

void* t_call_service(void *arg) {
    struct async_service_handle* handle = (struct async_service_handle*)arg;

    handle->result = call_service(handle->file_name, handle->result_buffer);
}

struct async_service_handle* initiate_async_service(char* file_name, char** result_buffer) {
    struct async_service_handle* handle = malloc(sizeof(struct async_service_handle));
    handle->file_name = file_name;
    handle->result_buffer = result_buffer;

    pthread_create(&(handle->pthread), NULL, t_call_service, (void*)handle);

    return handle;
}

int wait_for_results(struct async_service_handle* handle) {
    pthread_join(handle->pthread, NULL);

    free(handle);
    return 0;
}