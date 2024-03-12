#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>


#define TINY_FILE_KEY "tiny_file_key"
#define MAX_SERVINGS 32

enum message_type {
    // client
    SMS_SIZE_REQUEST,
    SERVICE_REQUEST,
    FILE_CONTENT_FILLED,
    SHM_USE_FINISHED,
    // server
    SMS_SIZE_RESPONSE,
    FILE_CONTENT_FILL_REQUEST,
    COMPRESS_DONE,
};

struct msg_buffer_client {
    long msg_type;

    enum message_type message_type;    
    int cli_msgqid;
};

struct msg_buffer_server {
    long msg_type;

    enum message_type message_type;

    int sms_size;
    int max_req_size;

    key_t shm_key;
    int server_private_msgqid;

    int compressed_size;
};

struct async_service_handle {
    pthread_t pthread;
    char* file_name;
    char** result_buffer;
    int result;
};

int call_sync_service(char* file_name, char** result_buffer);
struct async_service_handle* initiate_async_service(char* file_name, char** result_buffer);
int wait_for_results(struct async_service_handle* handle);