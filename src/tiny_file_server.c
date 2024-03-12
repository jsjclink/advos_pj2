#include "tiny_file.h"
#include "snappy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* global variables */
int current_servings;
int n_sms, sms_size, max_req_size;
int server_msgq_id;

struct snappy_env *se;

pthread_mutex_t lock;


/* functions */
void init_service(int argc, char *argv[]) {
    // parse cmd arguments
    if(argc == 5) {
        n_sms = atoi(argv[2]);
        sms_size = atoi(argv[4]);
        max_req_size = sms_size;
        while(max_req_size > 0 && snappy_max_compressed_length(max_req_size) > sms_size) {
            max_req_size -= 1;
        }
        printf("sms_size: %d, max_req_size: %d\n", sms_size, max_req_size);
    } else {
        printf("init failed\n");
        exit(1);
    }

    // init current_servings
    current_servings  = 0;

    // init snappy env
    if(!(se = malloc(sizeof(struct snappy_env)))){
        perror("snappy env malloc error");
        exit(1);
    }
    if(snappy_init_env(se) < 0){
        perror("snappy_init_env error\n");
        exit(1);
    }

    // get server msgq key
    key_t server_msgq_key;
    if((server_msgq_key = ftok(TINY_FILE_KEY, 255)) == -1) {
        perror("ftok error");
        exit(1);
    }
    // get server msgq
    if((server_msgq_id = msgget(server_msgq_key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget server error");
        exit(1);
    }

    struct msg_buffer_server msg;
    while(msgrcv(server_msgq_id, &msg, sizeof(msg), 1, IPC_NOWAIT) != -1);


    printf("server: init completed\n");
}

void sms_size_response(struct msg_buffer_client msg_client) {
    struct msg_buffer_server msg_server;

    printf("server: received sms_size_request\n");

    /* send response */
    msg_server.msg_type = 1;
    msg_server.message_type = SMS_SIZE_RESPONSE;
    msg_server.sms_size = sms_size;
    msg_server.max_req_size = max_req_size;

    if (msgsnd(msg_client.cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        perror("msgsnd error");
        exit(1);
    }
}

int get_id() {
    static int id = 0;
    id = (id + 1) % 255;
    return id;
}

void t_compress_service(int arg) {
    int cli_msgqid = arg;

    key_t server_private_msgq_key;
    int server_private_msgq_id;
    key_t shm_key;
    int shm_id;
    void* shm_ptr;
    struct msg_buffer_server msg_server;
    struct msg_buffer_client msg_client;

    printf("server: cli_msgqid: %d\n", cli_msgqid);
    printf("server: &cli_msgqid: %x\n", &cli_msgqid);

    /* send message to client. send server's private msgq_id, and file content fill request */
    // create server msgq
    pthread_mutex_lock(&lock);
    int key_id = get_id();
    pthread_mutex_unlock(&lock);

    if((server_private_msgq_key = ftok(TINY_FILE_KEY, key_id)) == -1) {
        perror("ftok error");
        exit(1);
    }
    if((server_private_msgq_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1) {
        perror("msgget client error");
        return -1;
    }
    // attach to new shm
    if((shm_key = ftok("shm_key", key_id)) == -1) {
        perror("ftok error");
        exit(1);
    }
    if ((shm_id = shmget(shm_key, sms_size, 0644 | IPC_CREAT)) == -1) {
        perror("shmget error");
        exit(1);
    }
    if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *)-1) {
        perror("shmat error");
        exit(1);
    }
    memset(shm_ptr, 0, sms_size);
    // setup msg
    msg_server.msg_type = 1;
    msg_server.message_type = FILE_CONTENT_FILL_REQUEST;
    msg_server.sms_size = sms_size;
    msg_server.max_req_size = max_req_size;
    msg_server.shm_key = shm_key;
    msg_server.server_private_msgqid = server_private_msgq_id;
    // send msg to client
    if(msgsnd(cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        perror("msgsnd error");
        return -1;
    }

    printf("shm_ptr: %x\n", shm_ptr);
    
    /* wait for msg from client(FILE_CONTENT_FILLED) */
    if(msgrcv(server_private_msgq_id, &msg_client, sizeof(msg_client), 1, 0) == -1) {
        perror("msgrcv error");
        exit(1);
    }

    printf("server: received FILE_CONTENT_FILLED\n");

    /* do compress */
    char* compression_output = malloc(sms_size * sizeof(char));
    int compressed_size = sms_size;
    // if(snappy_compress(se, shm_ptr, max_req_size, compression_output, &compressed_size) < 0){
    //     perror("snappy-c compress error");
    //     exit(1);
    // }
    memset(shm_ptr, 0, sms_size);
    memcpy(shm_ptr, compression_output, compressed_size);
    free(compression_output);

    // setup msg
    msg_server.msg_type = 1;
    msg_server.message_type = COMPRESS_DONE;
    msg_server.sms_size = sms_size;
    msg_server.max_req_size = max_req_size;
    msg_server.shm_key = shm_key;
    msg_server.server_private_msgqid = server_private_msgq_id;
    msg_server.compressed_size = compressed_size;
    // send msg to client
    if(msgsnd(cli_msgqid, &msg_server, sizeof(msg_server), 0) == -1) {
        perror("msgsnd error");
        return -1;
    }

    /* wait for msg from client(SHM_USE_FINISHED) */
    if(msgrcv(server_private_msgq_id, &msg_client, sizeof(msg_client), 1, 0) == -1) {
        perror("msgrcv error");
        exit(1);
    }

    printf("server: received SHM_USE_FINISHED\n");

    /* detach shm */
    shmctl(shm_id, IPC_RMID, NULL);
    shmdt(shm_ptr);
    /* remove msgq */
    msgctl(server_private_msgq_id, IPC_RMID, NULL);

    /* decrease current_servings counter */
    pthread_mutex_lock(&lock);
    current_servings -= 1;
    pthread_mutex_unlock(&lock);
}

void serve(struct msg_buffer_client msg_client) {
    key_t shm_key;
    int shm_id;
    void *shm_ptr;
    struct msg_buffer_server msg_server;

    printf("server: service request received\n");

    // if current service is full, push this request to msgq again
    if(current_servings == n_sms) {
        if (msgsnd(server_msgq_id, &msg_client, sizeof(msg_client), 0) == -1) {
            perror("msgsnd error");
            return -1;
        }

        printf("current service is full, push this request to msgq again\n");
        return;
    } else if(current_servings > n_sms) {
        // error. cleanup msgq (handle bug before exit)
        while(msgrcv(server_msgq_id, &msg_client, sizeof(msg_client), 1, IPC_NOWAIT) != -1);

        printf("fatal error. please restart this service\n");
        exit(1);
    }

    /* increase current_servings counter */
    pthread_mutex_lock(&lock);
    current_servings += 1;
    pthread_mutex_unlock(&lock);

    /* setup for pthread_create */
    pthread_t pthread;
    printf("server: start service. current_servings: %d\n", current_servings);

    pthread_create(&pthread, NULL, t_compress_service, msg_client.cli_msgqid);
    // do not wait
    if (pthread_detach(pthread) != 0) {
        perror("pthread_detach");
        exit(1);
    }
}



int main(int argc, char *argv[]) {
    struct msg_buffer_client msg_client;

    init_service(argc, argv);

    while(1) {
        // receive msg from client. 
        if(msgrcv(server_msgq_id, &msg_client, sizeof(msg_client), 1, 0) == -1) {
            perror("msgrcv error..");
            exit(1);
        }

        switch(msg_client.message_type) {
            case SMS_SIZE_REQUEST:
                sms_size_response(msg_client);
                break;
            case SERVICE_REQUEST:
                serve(msg_client);
                break;
            default: {
                printf("invalid message received\n");
            }
        };
    }
}