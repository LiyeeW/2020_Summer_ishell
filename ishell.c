#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>

#define COM_NUM 5       //amount of inner commands
#define FEAT_NUM 3      //amount of feature kind
#define MAX_TOK 32      //initial max volume of tok list
#define MAX_LEN 1024    //max volume of diractory's name
#define ERR_TOO_MANY "too many argument\n"  //error output (stderr)
#define ERR_MISSING "operand missing\n"     //error output (stderr)

typedef void (*funcp)(int, char**);     //inner cmd function pointer


    

void MainHandler(int s);
void SubHandler(int s);
void InitSigint(void (*handler)(int));

char* Readline(void);
void Free(int argc, char** argv);
int ArgCheck(int actual, int should_be);
char** GetStrTok(char* str, int* num);
int GetSubStr(char* str_a, char** str_b, char c);

int GetFeature(char* str_a, char** str_b);
void MyPipe(char* str_a, char* str_b);
int Redirect(char* str_b, int rop);
int InnerExecute(int argc, char** argv);
void MyExecute(char* str_a);

void MyExport(int argc, char** argv);
void MyEcho(int argc, char** argv);
void MyExit(int argc, char** argv);
void MyCd(int argc, char** argv);
void MyPwd(int argc, char** argv);


int main(int argc, char** argv){
    InitSigint(MainHandler);
    
    char *str_a, *str_b;
    int rop;    //record feature: 0 means none, 1 means re INPUT, 2 means re OUTPUT, 3 means pipe
    int prev_fd, redi_fd;   //record previous io and current io
    while(1){
        str_a = Readline();
        if(!str_a){      //ctrl-d ->quit
            putchar('\n');
            exit(0);
        }
        if(!strlen(str_a)){    //'\n' or main SIGINT
            continue;
        }
        rop = GetFeature(str_a, &str_b); 
        switch(rop){
            case 3:// execute in pipe
                MyPipe(str_a, str_b);
                break;
            case 2:
            case 1:// redirect before normal execute
                prev_fd = Redirect(str_b, rop);
                if(prev_fd==-1)
                    break;
            case 0:// normal excute
                MyExecute(str_a);
                if(rop){ //retrieve from redirection
                    if(dup2(prev_fd, rop-1)==-1) exit(-1);
                }
        }
    }
}

//main proceed's handler to SIGINT (fake interruption)
void MainHandler(int s){}

//subprocess's handler to SIGINT (true interruption)
void SubHandler(int s){
    putchar('\n');
    exit(0);
}

//install SIGINT handler 
void InitSigint(void (*handler)(int)){
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
}

char* Readline(void){
    int pfds[2], status;
    char* str;
    if(pipe(pfds)==-1) exit(-1);
    pid_t pid = fork();
    if(pid==-1) exit(-1);
    if(!pid){
        InitSigint(SubHandler);
        close(pfds[0]);
        str = readline(">> ");
        if(!str) exit(2);
        if(!strlen(str)) exit(3);
        write(pfds[1], str, strlen(str));
        exit(1);
    }
    else{
        waitpid(pid, &status, 0);
        if(!WEXITSTATUS(status)||WEXITSTATUS(status)==3) return "";
        if(WEXITSTATUS(status)==2) return NULL;
        close(pfds[1]);
        str = (char*)malloc(MAX_LEN*sizeof(char));
        read(pfds[0], str, MAX_LEN);
        return str;
    }
}

//examine numbers of arguments
int ArgCheck(int actual, int should_be){
    if(actual!=should_be){
        if(actual<should_be)
            fprintf(stderr, ERR_MISSING);
        else
            fprintf(stderr, ERR_TOO_MANY);
        return 1;
    }
    return 0;
}

//tok str by ' ', and num return amount
char** GetStrTok(char* str, int* num){
    if(!str){
        *(num) = 0;
        return NULL;
    }
    int cur_volume = MAX_TOK;
    char **tok_list = (char**)malloc(cur_volume*sizeof(char*));
    char **temp;
    int cur_num = 0;
    tok_list[cur_num] = strtok(str," ");
    while(1){
        for(cur_num=1;cur_num<cur_volume;cur_num++){
            tok_list[cur_num] = strtok(NULL, " ");
            if(!tok_list[cur_num])
                break;
        }
        if(cur_num==cur_volume){    //expand volume
            cur_volume += MAX_TOK;
            temp = (char**)realloc(tok_list, cur_volume*sizeof(char*));
            if(!temp)
                exit(-1);
            tok_list = temp;
        }
        else{
            *(num) = cur_num;
            return (char**)realloc(tok_list, (cur_num+1)*sizeof(char*));    //resize volume, end with NULL
        }
    }
}

//find c in str_a, and apart str_a into str_a and str_b by c
int GetSubStr(char* str_a, char** str_b, char c){
    char *str_cur = strchr(str_a, c);
    if(str_cur){
        *(str_b) = str_cur+1;   // record substring.
        *(str_cur) = 0;     // delete the substring in original string.
        return 0;
    }
    *(str_b) = NULL;
    return 1;
}

//detect feature, and may change str_a and str_b
int GetFeature(char* str_a, char** str_b){
    char *str_cur;
    int feat_char[] = {'<', '>', '|'};
    for(int i=0;i<FEAT_NUM;i++){
        if(GetSubStr(str_a, str_b, feat_char[i])==0)
            return i+1;
    }
    *(str_b) = NULL;
    return 0;
}

//execute with pipe
void MyPipe(char* str_a, char* str_b){
    pid_t pid_a, pid_b;
    int Argc_a, Argc_b, pfds[2], status, prev_in;
    char **Argv_a, **Argv_b;
    if(pipe(pfds) ==-1) exit(-1);
    pid_a = fork();
    if(pid_a==-1) exit(-1);
    prev_in = dup2(STDIN_FILENO, prev_in);
    if(!pid_a){     //process a
        InitSigint(SubHandler);
        dup2(pfds[1], STDOUT_FILENO);
        close(pfds[0]);
        MyExecute(str_a);
        exit(1);
    }
    else{   //process a - str_b
        waitpid(pid_a, &status, 0);
        if(!WEXITSTATUS(status)) return;
        dup2(pfds[0], STDIN_FILENO);
        close(pfds[1]);
        MyExecute(str_b);
        dup2(prev_in, STDIN_FILENO);
        return;
    }
}

//return previous io copy , -1 means error
int Redirect(char* str_b, int rop){
    rop--;  //0 is stdin, 1 is stdout
    int num, redi_fd, prev_fd;
    char** filename = GetStrTok(str_b, &num);
    if(ArgCheck(num, 1)){
        if(filename) free(filename);
        return -1;
    }
    redi_fd = open(*(filename), O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG);
    if(redi_fd==-1){
        fprintf(stderr, "%s: file error\n", *(filename));
        if(filename) free(filename);
        return -1;
    }
    if(dup2(rop, prev_fd)==-1) exit(-1);
    if(dup2(redi_fd, rop)==-1) exit(-1);
    if(filename) free(filename);
    return prev_fd;
}

//find inner function and finish it
int InnerExecute(int argc, char** argv){
    char* inner_text[] = {"myexport", "myecho", "myexit", "mycd", "mypwd"};
    funcp inner_func[] = {MyExport, MyEcho, MyExit, MyCd, MyPwd};
    for(int i=0;i<COM_NUM;i++){
        if(!strcmp(argv[0], inner_text[i])){
            (*inner_func[i])(argc, argv);
            return 0;
        }
    }
    return 1;
}

//excute inner or outer command with no pipe
void MyExecute(char* str_a){
    int Argc;
    char** Argv = GetStrTok(str_a, &Argc);
    if(InnerExecute(Argc, Argv)==0)
        return;
    pid_t pid = fork();
    if(pid==-1) exit(-1);
    if(!pid){
        InitSigint(SubHandler);
        if(execvp(Argv[0], Argv)==-1){
            perror("execvp");
        };
        exit(1);
    }
    else{
        waitpid(pid, NULL, 0);
        return;
    }
}

//diffrent from true export, only can add PATH(argv[1])
//ex: myexport /home => export PATH=/home:$PATH
void MyExport(int argc, char** argv){
    if(ArgCheck(argc, 2)) return;
    char* prev_path = getenv("PATH");
    char cur_path[MAX_LEN];
    sprintf(cur_path, "%s:%s", argv[1], prev_path);
    int status = setenv("PATH", cur_path, 2);
    Free(argc, argv);
    return;
}

void MyEcho(int argc, char** argv){
    int len;
    for(int i=1;i<argc;i++){
        len = strlen(argv[i]);
        if(*(argv[i]+len-1)=='\''||*(argv[i]+len-1)=='\"'){//delete " or ' *(tok_list[i]+len-1) = 0;
             *(argv[i]+len-1) = 0;
        }
        if(*(argv[i])=='\''||*(argv[i])=='\"'){
                argv[i] = argv[i] + 1;
        }
        if(*(argv[i])=='$'){    // although it's diffrent from truth, still leave it for convenience
            argv[i] = getenv(argv[i]+1);
            if(!argv[i])
                argv[i] = "";
        }
    }
    for(int i=1;i<argc;i++){
        write(STDOUT_FILENO, argv[i], strlen(argv[i]));
        if(i!=argc-1)
            write(STDOUT_FILENO, " ", 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    Free(argc, argv);
    return;
}

void MyExit(int argc, char** argv){
    exit(0);
}

void MyCd(int argc, char** argv){
    if(ArgCheck(argc, 2)){
        Free(argc, argv);
        return;
    }
    struct stat st;
    stat(argv[1],&st);
    if (S_ISDIR(st.st_mode))
        chdir(argv[1]);
    else
        fprintf(stderr, "%s: file error\n", argv[0]);
    Free(argc, argv);
    return;
}

void MyPwd(int argc, char** argv){
    if(argc>1){
        fprintf(stderr, ERR_TOO_MANY);
    }
    char* work_dir = (char*)malloc(MAX_LEN*sizeof(char));
    getcwd(work_dir, MAX_LEN);
    write(STDOUT_FILENO, work_dir, strlen(work_dir));
    write(STDOUT_FILENO, "\n", 1);
    free(work_dir);
    return;
}

//only free argv[0] becaause of strtok 
void Free(int argc, char** argv){
    if(!argv) return;
    if(argv[0])
        free(argv[0]);
    free(argv);
}
