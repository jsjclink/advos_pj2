#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>


#define TINY_FILE_KEY "tiny_file_key"
#define MAX_SERVINGS 32

enum message_type {
    SHM_REQUEST,
    FILE_CONTENT_FILLED,
    REQUEST_REJECT,
    SHM_RESPONSE,
    COMPRESS_RESPONSE,
};

struct msg_buffer_client {
    long msg_type;

    enum message_type message_type;    
    long shm_size;
    int cli_msgqid;
};

struct msg_buffer_server {
    long msg_type;

    enum message_type message_type;
    key_t shm_key;
};

struct current_serving_state {
    int valid;
    int cli_msgqid;
    void* shm_ptr;
};