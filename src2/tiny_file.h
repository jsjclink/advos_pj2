#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>


#define TINY_FILE_KEY "tiny_file_key"


struct msg_buffer_client {
    long msg_type;
    
    key_t shm_key;
    long shm_size;

    int cli_msgqid;
};

struct msg_buffer_server {
    long msg_type;
    long comp_size;
};