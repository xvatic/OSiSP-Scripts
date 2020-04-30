#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<errno.h>
#include<limits.h>//realpath
#include<libgen.h>//basename

char *Application;
FILE *outputfile = NULL;

void print_error(const char *s_name, const char *msg, const char *f_name)
{
    fprintf(stderr, "%s: %s %s\n", s_name, msg, (f_name)? f_name : "");
    if (outputfile) {
        fprintf(outputfile, "%s: %s %s\n", s_name, msg, (f_name)? f_name : "");
    }
}


int ProcessFolder( FILE *outputfile, char *currPath){
    DIR *currDir;
    currDir = opendir(currPath);
    if(currDir == NULL) {
        print_error(Application, strerror(errno), currPath);
        errno = 0;
        return -1;
    }
    struct dirent *entr;
    struct stat buf;
    char *file = alloca(strlen(currPath) + NAME_MAX + 2);
    if(file==NULL){
        print_error(Application, strerror(errno), currPath);
        return -1;
    }
    long int sum = 0;
    int count = 0;
    int maxSize = -1;
    char *maxFile = alloca(NAME_MAX);
    if(maxFile==NULL){
        print_error(Application, strerror(errno), currPath);
        return -1;
    }
    maxFile[0] = 0;
    errno = 0;
    while(entr = readdir(currDir)){
        if(strcmp(".", entr->d_name) && strcmp("..", entr->d_name)){
            strcpy(file, currPath);
            strcat(file, "/");
            strcat(file, entr->d_name);
            if (lstat(file,&buf) == -1) {
                print_error(Application, strerror(errno), file);
                return -1;
            }
            if(!S_ISDIR(buf.st_mode)){
                if(buf.st_size > maxSize){
                    maxSize = buf.st_size;
                    strcpy(maxFile, basename(file));
                }
                sum+=buf.st_size;
                count++;
            }
            if(S_ISDIR(buf.st_mode)){
                ProcessFolder(outputfile, file);
            }
        }
    }
    
    if(errno!=0){
        print_error(Application, strerror(errno), currPath);
        return -1;
    }
    
    printf("%s %d %ld %s\n", currPath, count, sum, maxFile);
    fprintf(outputfile,"%s %d %ld %s\n", currPath, count, sum, maxFile);
    if(closedir(currDir)==-1){
        print_error(Application, strerror(errno), currPath);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv){
    Application = alloca(strlen(basename(argv[0])));
    strcpy(Application, basename(argv[0]));
    if(argc<3){
        print_error(Application, "Not enough arguments.", 0);
        return -1;
    }
    
    if((outputfile = fopen(argv[2],"w")) == NULL){
        print_error(Application, "directory Error", argv[1]);
        return -1;
    }
    
    if(realpath(argv[1], NULL) == NULL) {
        print_error(Application, "outputfile Error", argv[2]);
        return -1;
    }
    
    ProcessFolder(outputfile, realpath(argv[1], NULL));
    
    if(fclose(outputfile)!=0){
        print_error(Application, "closing outputfile Error", realpath(argv[2], NULL) );
        return -1;
    }
    return 0;
}


