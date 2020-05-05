#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<alloca.h>
#include<malloc.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<libgen.h>
#include<limits.h>
#include<errno.h>
#include<unistd.h>
#include<dirent.h>

char delims[6] = {' ','\n','\t','\v','\f','\r'};
char *Application;
FILE *outputfile = NULL;
int proc_amount = 0, max_proc_amount = 0;
unsigned long lOC_SIZE = 8192;
char *block_read;

int check_delims(char delim)
{
    for (int i=0; i<=5; i++){
        if (delim == delims[i])
        {
            return -1;
        }
        
    }
    return 1;
}

void print_error(const char *s_name, const char *msg, const char *f_name)
{
    fprintf(stderr, "%s: %s  %s\n", s_name, msg, (f_name)? f_name : "");
    if (outputfile) {
        fprintf(outputfile, "%s: %s  %s\n", s_name, msg, (f_name)? f_name : "");
    }
}

void print_result(int pid, char *path,unsigned int byte, unsigned int word )
{
    printf("%d %s %u %u\n",pid, path, byte, word);
    if (outputfile) {
        fprintf(outputfile,"%d %s %u %u\n",pid, path, byte, word);
    }
}

char* read_all_path(char *file, char *path, char *d)
{
    strcpy(file, path);
    if(strcmp(file,"/")!=0)
        strcat(file, "/");
    strcat(file, d);
    return file;
}

int count(FILE *outputfile,char *fullpath){
    int f = open(fullpath, O_RDONLY);
    if(f == -1){
        print_error(Application, strerror(errno), fullpath);
        exit(-1);
    }
    char d=-1;
    unsigned word=0, byte=0;
    block_read = malloc(lOC_SIZE);
    
    int block = 0;
    while((block = read(f, block_read, lOC_SIZE)) != 0){
        if(block == -1){
            print_error(Application, strerror(errno), fullpath);
            exit(-1);
        }
        for(int i = 0;i<block;i++){
            
            byte++;
            if (check_delims(block_read[i]) == -1)
            {
                d = -1;
                continue;
            }
            
            
            if(d==-1)
            {
                d = 1;
                word++;
            }
            
        }
    }
    free(block_read);
    print_result((int)getpid(), fullpath, byte, word);
    if(close(f) == -1){
        print_error(Application, strerror(errno), fullpath);
        exit(-1);
    }
    exit(0);
}

void ProcessFolder(FILE *outputfile, char *currPath){
    DIR *currDir;
    if((currDir = opendir(currPath)) == NULL) {
        print_error(Application, strerror(errno), currPath);
        errno = 0;
        return;
    }
    struct dirent *entr;
    char *currFile = alloca(strlen(currPath) + NAME_MAX + 2);
    if(currFile==NULL){
        print_error(Application, strerror(errno), currPath);
        return;
    }
    errno = 0;
    while(entr = readdir(currDir)){
        if(strcmp(".", entr->d_name) && strcmp("..", entr->d_name)){
            
            currFile = read_all_path(currFile, currPath, entr->d_name);
            struct stat fileinfo;
            if (lstat(currFile,&fileinfo) == -1) {
                print_error(Application, strerror(errno), currPath);
                return;
            }
            if(S_ISDIR(fileinfo.st_mode)){
                ProcessFolder(outputfile, currFile);
            }
            int proc;
            if(S_ISREG(fileinfo.st_mode)){
                if(proc_amount==max_proc_amount) {
                    wait(&proc);
                    proc_amount--;
                }
                switch(fork()){
                    case (pid_t)-1:
                        print_error(Application, strerror(errno), currPath);
                        errno = 0;
                        break;
                    case (pid_t)0:
                        count(outputfile, currFile);
                        break;
                    default:
                        proc_amount++;
                        break;
                }
            }
        }
    }
    if(errno!=0){
        print_error(Application, strerror(errno), currPath);
        errno = 0;
        return;
    }
    
    if(closedir(currDir)==-1){
        print_error(Application, strerror(errno), currPath);
        errno = 0;
        return;
    }
    return;
}

int main(int argc, char**argv){
    Application = alloca(strlen(basename(argv[0])));
    strcpy(Application, basename(argv[0]));
    
    
    if(argc<3){
        print_error(Application, "not enough arguments.", 0);
        return -1;
    }
    
    max_proc_amount = atoi(argv[3]);
    if(max_proc_amount<=0){
        print_error(Application, "increase N", 0);
        return -1;
    }
    
    char *res;
    if((res = realpath(argv[1], NULL)) == NULL) {
        print_error(Application, "directory error", argv[1]);
        return -1;
    }
    
    if((outputfile = fopen(argv[2],"w")) == NULL){
        print_error(Application, "outputfile error", argv[1]);
        return -1;
    }
    
    
    ProcessFolder(outputfile, res);
    
    if(fclose(outputfile)!=0){
        print_error(Application, "closing outputfile error", realpath(argv[2], NULL) );
        return -1;
    }
    
    
    while (wait(NULL) != -1) { }
    
    return 0;
}
