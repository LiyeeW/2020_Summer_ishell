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
    char *dir_name, *buf_name;
    if(argc>2){
        fprintf(stderr, ERR_TOO_MANY);
        return 0;
    }
    if(argc==1||strlen(argv[1])==0){
        buf_name = (char*)malloc(MAX_LEN*sizeof(char));
        if(!buf_name)
            exit(-1);
        dir_name = getcwd(buf_name, MAX_LEN);
    }
    else{
        dir_name = argv[1];
    }
    DIR* dp=opendir(dir_name);
    if(!dp){
        fprintf(stderr, "%s: file error\n", dir_name);
        return 0;
    }
    struct dirent* de=readdir(dp), *temp;
    struct stat s;
    while(de){
        if(de->d_name[0]=='.'){
            de = readdir(dp);
            continue;
        }
        write(STDOUT_FILENO, de->d_name, strlen(de->d_name));
        temp = de;
        de = readdir(dp);
        write(STDOUT_FILENO, "  ", 2);  
    }
    write(STDOUT_FILENO, "\n", 1); 
    closedir(dp);
    return 0;
}
