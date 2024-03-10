#include "tiny_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char * file_name;
    char * state;
    char buf[256];
    int mult = 0;
    int count = 0;
    void * result;
    struct async_service_handle* handler;

    FILE *file;
    //Parsing Arguments
    if(argc==5){
        if(strcmp(argv[1],"--file")==0)
            mult = 0;
        else if(strcmp(argv[1],"--files")==0)
            mult = 1;
        else{
            printf("Wrong Input\n");
            exit(1);
        }
        file_name = argv[2];
        state = argv[4];
    }else{
        printf("Wrong Input\n");
        exit(1);
    }

    if(mult){
        if ((file = fopen(file_name, O_RDWR)) == -1) {
            printf("open failed\n");
            exit(1);
        }
        while (fgets(buf, sizeof(buf), file)) {
            if(call_sync_service(file_name,result)<0){
                printf("TF Service Error\n");
                exit(1);
            }
            //TODO: Do something with result?
        }
        fclose(file);
    }

    else{
        if(!(handler = initiate_async_service(file_name,result))){
            printf("TF Service Error\n");
            exit(1);
        }
        for(int i = 0; i <10; i++){
            printf("HI %d\n",i);
            sleep(1);
        }
        if(wait_for_results(handler) < 0 ){
            printf("TF Service Error\n");
            exit(1);
        }
        printf("client: got back from api: \"%s\"\n", result);
        if(result != NULL) printf("TF Success!\n");
        else printf("TF Failure...\n");
    }
}