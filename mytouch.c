#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <dirent.h>
#include <setjmp.h>
#include <string.h>
#include <utime.h>
#include <fcntl.h>

#define COM_NUM 7
#define FEAT_NUM 3
#define MAX_TOK 32
#define MAX_LEN 1024
#define ERR_TOO_MANY "too many argument\n"
#define ERR_MISSING "argument missing\n"

int main(int argc, char** argv){
    int status;
    if(argc<2){
        fprintf(stderr, ERR_MISSING);
        return 0;
    }
    else if(argc>2){
        fprintf(stderr, ERR_TOO_MANY);
        return 0;
    }
    int fd = open(argv[1], O_RDONLY|O_CREAT, S_IRWXU|S_IRWXG);
    if(fd==-1){
        fprintf(stderr, "%s: file error\n", argv[1]);
        return 0;
    }        
    status = utime(argv[1], NULL);
    if(status==-1)
        perror("touch");
    return 0;
}
