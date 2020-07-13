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

#define COM_NUM 7
#define FEAT_NUM 3
#define MAX_TOK 32
#define MAX_LEN 1024
#define ERR_TOO_MANY "too many argument\n"
#define ERR_MISSING "argument missing\n"

int main(int argc, char** argv){
    if(argc<2){
        fprintf(stderr, ERR_MISSING);
        return 0;
    }
    else if(argc>2){
        fprintf(stderr, ERR_TOO_MANY);
        return 0;
    }
    char* filename = argv[1];
    if(mkdir(filename, 777)==-1){
        perror("mkdir");
    }
    return 0;
}
