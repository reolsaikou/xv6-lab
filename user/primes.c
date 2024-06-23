#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(void){
    int nums[35];
    for(int i=0;i<35;i++) nums[i] = (i>=34? -1:i+2);
    // for(int i=0;i<36;i++) fprintf(2, "the num[%d]: %d\n", i, nums[i]);
    // int *pr=nums;
    // for(int i=0;i<36;i++){
    //     if(*(pr+i) == -1) break;
    //     printf("%d\n" ,*(pr+i));
    // }
    int pSieve[2][2];
    pipe(pSieve[0]);pipe(pSieve[1]);
    if(fork()==0){
        close(pSieve[0][1]);close(pSieve[1][0]);
        while(1){
            char buf[500];
            read(pSieve[0][0], buf, 500);
            char *token = strtok(buf, " ");
            int childNums[35];
            int id=0;
            while(token !=0){
                childNums[id++] = atoi(token);
                // printf("childNums[%d]: %d\n",id-1, childNums[id-1]);
                token = strtok(0, " ");
            }
            childNums[id++]=-1;
            int p=childNums[0];
            printf("prime %d\n", p);
            if(childNums[1]==-1){
                memset(buf, 0, sizeof(buf));
                write(pSieve[1][0], buf, 500);
                break;
            }
            int sendNums[35];
            for(int i=0,j=0;i<35;i++){
                // printf("childNums[%d]: %d\n",i, childNums[i]);
                if(childNums[i]==-1){
                    sendNums[j]=-1;
                    break;
                }
                if(childNums[i]%p!=0){
                    sendNums[j++]=childNums[i];
                    // printf("sendNum[%d]: %d\n",j-1, sendNums[j-1]);
                }
            }
            char *curPos = buf;
            for(int i=0;i<34;i++){
                if(sendNums[i]==-1) break;
                curPos += sprintf(curPos, "%d ", sendNums[i]);
            }
            write(pSieve[1][1], buf, 500);
        }
        close(pSieve[0][0]);close(pSieve[1][1]);
        // printf("child exit\n");
        exit(0);
    }else{
        close(pSieve[0][0]);close(pSieve[1][1]);
        char buf[500];
        char *curPos = buf;
        for(int i=0;i<34;i++){
            curPos += sprintf(curPos, "%d ", nums[i]);
        }
        *(curPos++) = '\0';
        // printf("buf:\n%s\n",buf);
        while(1){
            write(pSieve[0][1], buf, 500);
            read(pSieve[1][0], buf, 500);
            if(strcmp(buf,0)) break;
        }
        close(pSieve[0][1]);close(pSieve[1][0]);
    }
    wait(0);
    // printf("parent exit\n");
    exit(0);
}
