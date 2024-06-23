#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(void){
    char s[]="1\n2";
    for(int i=0;s[i]!=0;i++){
        printf("%d\n",s[i]);
    }
    printf(s);
    exit(0);
}