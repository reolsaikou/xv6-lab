#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void){
    int p1[2],p2[2];
    pipe(p1);pipe(p2);
    
    if(fork() == 0){
        close(p1[1]);
        int childPid = getpid();
        char buf[100];
        int IsRead=read(p1[0], buf, 4);
        if(IsRead == -1){
            fprintf(2, "read error\n");
            exit(1);
        }
        if(strcmp(buf, "ping") == 0){
            fprintf(2, "%d: received %s\n", childPid, buf);
        } else{
            fprintf(2, "%d: not received %s\n", childPid, buf);
            exit(1);
        }
        close(p1[0]);
        write(p2[1], "pong", 4);
        close(p2[1]);
        exit(0);
    } else {
        int IsWrite=write(p1[1], "ping", 4);
        close(p1[1]);
        if(IsWrite == -1){
            fprintf(2, "write error\n");
            exit(1);
        }
        char buf[100];
        close(p2[1]);
        read(p2[0], buf, 4);
        int parentPid = getpid();
        if(strcmp(buf, "pong") == 0){
            fprintf(2, "%d: received %s\n", parentPid, buf);
        } else exit(1);
        close(p2[0]);
    }
    wait(0);
    exit(0);
}