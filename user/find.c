#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"



void find(char *path,char *tag){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    // printf("pre open\n");
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    
    // printf("pre fstat\n");
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        return;
    }

    if(st.type != T_DIR){
        fprintf(2, "find: type is wrong, not directory");
        return;
    }
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        fprintf(2, "find: path too long\n");
        return;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    // printf("pre read\n");
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        // printf("pre memmove\n");
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        // printf("pre stat\n");
        if(stat(buf, &st) < 0){
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        // printf("pre compare type\n");
        // printf("de.name:%s, st.type:%d\n", de.name, st.type);
        if(st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0){
            find(buf, tag);
            continue;
        }
        // printf("pre printf\n");
        if(strcmp(de.name, tag) == 0)
            printf("%s\n", buf);
    }
    close(fd);
}


int main(int argc, char *argv[]){

    if(argc < 3 || argc > 3){
        exit(0);
    }
    // printf("pre find\n");
    find(argv[1], argv[2]);
    exit(0);
}
