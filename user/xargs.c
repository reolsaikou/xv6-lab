#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


#define MAXARGS 15

char whitespace[] = " \t\r\v\n";
char symbols[] = "<|>&;()";

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s) && !strchr("\n", *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}


int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s) )
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  
  while(s < es && strchr(whitespace, *s) && !strchr("\n", *s))
    s++;
  // printf("char:%d\n",*s);
  *ps = s;
  return ret;
}


int main(int argc,char *argv[]){
    for(int i=0;i<argc;i++){
        // printf("argv[%d]:%s\n", i,argv[i]);
    }
    // printf("size of argv:%d", sizeof(argv));
    char buf[100];
    // gets(buf,sizeof(buf));
    read(0, buf, 100);
    char *s=buf;
    int buflen=0;
    for(int i=0;i<100&&buf[i]!=0;i++) buflen++;
    // printf("string:%s",s);
    // printf("buflen:%d\n",buflen);
    char *es;
    int tok;
    es = s+buflen;
    // peek(&s, es, "\"");
    // printf("&c%d\n",*s);
    if(*s==34) s++;
    if(*(es-2)==34){
      es--;*es=0;
      *(es-1)='\n';
    }
    char buffer[100];
    sprintf(buffer, s);
    // printf("s\n");
    // printf(s);
    // printf("buffer%s", buffer);
    // peek(&s, es, "\"");
    char *eargv[MAXARGS];
    argc=0;
    char **exargv = malloc((MAXARGS + 1) * sizeof(char*));
    // printf("argv[i]:%s", argv[1]);
    for(int i=0,j=1;i<MAXARGS&&argv[j] != 0;i++,j++,argc++){
      // printf("%d%d\n",i, j);
      if(strcmp(argv[j],"-n")==0){
        i--;j++;
        // printf("zhongtu\n");
        continue;
      }
      exargv[i]=argv[j];
      eargv[i]=argv[j]+sizeof(argv[j]);
      // printf("argv[0]:%s\n", argv[i]);
    }
    int oargc=argc;
    // printf("string:%s\n",s);
    char *q, *eq;
    while(!peek(&s, es, "|)&;")){
      // printf("count\n");
      // printf("string:%s\n",s);
      for(int i=0;i<buflen;i++){
        // printf("%d ", *(s+i));
      }
      // printf("\n");
      
      // char *end=0;
      if(peek(&s, es, "\n")){
        // printf("%s\n", *exargv[1]);
        // printf("%p\n", &argv[argc]);
        exargv[argc]=0;
        s++;
        // printf("max_argc:%d\n",argc);
        for(int i=0;i<argc;i++){
          *eargv[i]=0;
          // printf("argv[%d]:%s\n", i, exargv[i]);
        }
        // printf("\n");
        if(fork()==0){
          exec(exargv[0],exargv);
          // exit(0);
        }else{
          wait(0);
        }
        argc=oargc;
      }
      if((tok=gettoken(&s, es, &q, &eq)) == 0){
        // printf("s:",s);
        break;
      }
        // break;
      if(tok != 'a')
        panic("syntax");
      // printf("test\n");
      exargv[argc] = q;
      eargv[argc] = eq;
      // *eargv[argc]=0;
      argc++;
      if(argc >= MAXARGS)
        panic("too many args");
    }
    // argv[argc]=0;
    // eargv[argc]=0;
    // printf("argv[argc]:%s\n",eargv[argc]);
    for(int i=0;i<MAXARGS&& !exargv[i];i++){
      // printf("argv[i]:%s", *argv[i]);
    }
    if(fork() == 0){
    }else{
    }
    exit(0);
}

