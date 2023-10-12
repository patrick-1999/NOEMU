#include <common.h>
#include "syscall.h"
void halt(int code);
void write(uintptr_t *a){
  if(a[1]==1){
    // stdout
    char *buf = (char*)a[2];

    for(int i=0;i<a[3];i++){
      putch(*(buf+i));
    }
  }
  if(a[1]==2){
    // stderror
  }
}
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  // printf("do syscall:%d\n",a[0]);
  switch (a[0]) {
    case 0:
      // printf("a[1]:%d\n",a[1]);
      // printf("a[2]:%d\n",a[2]);
      // printf("a[3]:%d\n",a[3]);
      halt(0);
      break;
    case 1:
      yield();
      break;
    case 4:
      // do write
      write(a);
      // printf("a[1]:%d\n",a[1]);
      // printf("a[2]:%x\n",a[2]);
      // printf("a[3]:%d\n",a[3]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
