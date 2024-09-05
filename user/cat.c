#include "libc.h"

int main(int argc, char *argv[]){
    char buff[128];

    if(argc < 2){
        return -1;
    }

    int fd = open(argv[1], 'r');

    while(read(fd, &buff, 128) == 128 ){
        write(1,&buff,128);
    }
}