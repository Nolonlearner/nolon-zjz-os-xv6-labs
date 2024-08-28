#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READED 0
#define WRITEEND 1
int
main(int argc, char **argv)
{
   int pid;
   int p1[2],p2[2];
   char buf[1];
   
   pipe(p1);//parent -> child
   pipe(p2);//child -> parent
   
   pid = fork();
   
   if(pid<0) { exit(1); }
   else if(pid==0)
   {//child
       close(p1[WRITEEND]);
       close(p2[READED]);
       read(p1[READED],buf,1);//read a btye from p1
       printf("%d:received ping\n", getpid());
       write(p2[WRITEEND]," ",1);//wrire a byte to p2 
       close(p2[WRITEEND]);
       close(p1[READED]);
       exit(0);
   }
   else{
   //parent
      close(p1[READED]);
      close(p2[WRITEEND]);
      write(p1[WRITEEND], "x", 1); 
      read(p2[READED],buf,1);
      printf("%d: received pong\n", getpid());
      close(p2[READED]);
      close(p1[WRITEEND]);
   }
   exit(0);
  
}
