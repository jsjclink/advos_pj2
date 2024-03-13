#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char * file_name;
    char * state;
    char file_name_buf[256];
    int mult = 0;
    int count = 0;
    void *result;
    struct async_service_handle* handler;
    struct async_service_handle* handlers[256];
    FILE *file;

    // Parsing Arguments
    if(argc == 5) {
        if(strcmp(argv[1], "--file") == 0)
            mult = 0;
        else if(strcmp(argv[1], "--files") == 0)
            mult = 1;
        else {
            printf("Wrong Input\n");
            exit(1);
        }
        file_name = argv[2];
        state = argv[4];
    } else {
        printf("Wrong Input\n");
        exit(1);
    }

    // SYNC | ASYNC
    if(strcmp(state, "SYNC") == 0) {
        if(mult) {
            if ((file = fopen(file_name, "r")) == -1) {
                printf("open failed\n");
                exit(1);
            }
            while (fgets(file_name_buf, sizeof(file_name_buf), file)) {
                file_name_buf[strcspn(file_name_buf, "\n")] = 0;
                if(call_sync_service(file_name_buf, &result)<0){
                    printf("TF Service Error\n");
                    exit(1);
                }
                //TODO: Do something with result?
            }
            fclose(file);
        }
        else {
            if(call_sync_service(file_name, &result)<0){
                printf("TF Service Error\n");
                exit(1);
            }
        }
    }
    else if(strcmp(state, "ASYNC") == 0) {
        if(mult) {
            if ((file = fopen(file_name, "r")) == -1) {
                printf("open failed\n");
                exit(1);
            }
            int handler_idx = 0;
            while (fgets(file_name_buf, sizeof(file_name_buf), file)) {
                file_name_buf[strcspn(file_name_buf, "\n")] = 0;
                if((handlers[handler_idx++] = initiate_async_service(file_name_buf, &result)) < 0){
                    printf("TF Service Error\n");
                    exit(1);
                }
                //TODO: Do something with result?
            }
	    for (int i=0;i<10;i++){
		printf("Some task:%d\n",i);
	    }
            for(int i = 0; i < handler_idx; i++) {
                if(wait_for_results(handlers[i]) < 0) {
                    printf("TF Service Error\n");
                    exit(1);
                }
            }

            fclose(file);
        }
        else {
            if((handler = initiate_async_service(file_name, &result)) < 0){
                printf("TF Service Error\n");
                exit(1);
            }
	    for(int i=0;i<10;i++){
		printf("Some task:%d\n",i);
	    }
            if(wait_for_results(handler) < 0){
                printf("TF Service Error\n");
                exit(1);
            }
        }
    } else {
        printf("Wrong Input\n");
        exit(1);
    }


}
