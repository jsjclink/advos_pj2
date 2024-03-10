#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/* Calling SYNC TINYFILE 
   Input: file name , result buffer
   Output: Returns 0 if successful
*/
int call_sync_service(char* file_name, char** result_buffer) {
    key_t server_msgq_key;
    int server_msgq_id, client_msgq_id;
    struct msg_buffer_server msg_server;
    struct msg_buffer_client msg_client;

    key_t shm_key;
    int shm_id;
    int sms_size;
    void* shm_ptr;
    char* file_buffer;
    int count = 0;
    int fd;
    struct stat file_stat;

    time_t start_time, end_time;
    double elapsed_time;

    /* init */
    start_time = time(NULL);
    // get server msgq key
    if((server_msgq_key = ftok(TINY_FILE_KEY, 's')) == -1) {
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

    printf("client: init completed\n");

        /* open file */
    if ((fd = open(file_name, O_RDWR)) == -1) {
        perror("open failed");
        return -1;
    }

    if (fstat(fd, &file_stat) == -1) {
        perror("fstat failed");
        close(fd);
        return -1;
    }
    printf("client: file open completed\n");
    file_buffer = malloc(file_stat.st_size * sizeof(char));
    if (read(fd, file_buffer, file_stat.st_size) == -1) {
        perror("read failed");
        shmdt(shm_ptr);
        close(fd);
        return -1;
    }
    
    close(fd);

    printf("client: file content copy completed\n");
    /* request shared memory to server */
    msg_client.msg_type = 1;
    msg_client.cli_msgqid = client_msgq_id;
    msg_client.message_type = SMS_SIZE_REQUEST;
    msg_client.shm_size = file_stat.st_size;

    if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
        perror("msgsnd error");
        return -1;
    }

    printf("client: requested sms_size to server\n");

    /* wait for shared memory response */
    printf("client: waiting for sms_size response\n");

    if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
        perror("msgrcv error");
        return -1;
    }

    if(msg_server.message_type == REQUEST_REJECT) {
        printf("client: request rejected. abort\n");
        return -1;
    }

    printf("client: received sms_size response\n");
    sms_size = msg_server.sms_size;
    *result_buffer = malloc(32 + file_stat.st_size * sizeof(char) + file_stat.st_size * sizeof(char)/6);
    // HERE
    int itr =  (file_stat.st_size + sms_size -1)/ sms_size;
    for(int i = 0 ; i <itr; i++){
        /* copy file content to shared memory */
        msg_client.msg_type = 1;
        msg_client.cli_msgqid = client_msgq_id;
        msg_client.message_type = SHM_REQUEST;
        msg_client.shm_size = sms_size;

        if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }

        printf("client: requested shm_sgm to server\n");

        /* wait for shared memory response */
        printf("client: waiting for shm_sgm response\n");

        if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
            perror("msgrcv error");
            return -1;
        }
        printf("client: got shm_req response\n");

        if((shm_id = shmget(msg_server.shm_key, sms_size, 0644)) == -1) {
            perror("shmget error");
            return -1;
        }
        if((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
            perror("shmat error");
            return -1;
        }
        printf("client: attached to shm\n");
        /* write file content to shared memory*/

        memcpy(shm_ptr, file_buffer + sms_size * i, sms_size);
        printf("client: moved data to shm\n");
        /* send file_content_filled alert to server */
        msg_client.msg_type = 1;
        msg_client.cli_msgqid = client_msgq_id;
        msg_client.message_type = FILE_CONTENT_FILLED;

        if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }
        
        /* wait for response */
        printf("client: waiting for file compress response\n");

        if (msgrcv(client_msgq_id, &msg_server, sizeof(msg_server), 1, 0) == -1) {
            perror("msgrcv error");
            return -1;
        }

        printf("client: received file compress response\n");

        /* ... */
        
        memcpy(*result_buffer + count, shm_ptr, msg_server.sms_size);
        //printf("client: res_buffer: \"%d\"\n", msg_server.sms_size);
        count += msg_server.sms_size;
        shmctl(shm_id,IPC_RMID,NULL);
        shmdt(shm_ptr);
    }
    end_time = time(NULL);
    elapsed_time = difftime(end_time, start_time);
    printf("Elapsed time: %.10f seconds\n", elapsed_time);
    printf("client: res_buffer: \"%s\"\n", *result_buffer);
    return 0;
}

void* t_call_service(void *arg) {
    struct async_service_handle* handle = (struct async_service_handle*)arg;

    handle->result = call_sync_service(handle->file_name, handle->result_buffer);
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