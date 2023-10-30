#include <common.h>
#include "fs.h"
#include "syscall.h"
#include <sys/time.h>



void halt(int code);
int fs_open(const char *pathname, int flags, int mode);
int fs_close(int fd);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);

void write(uintptr_t *a){
  // printf("a1:%d\n",a[1]);
  // printf("a2:%x\n",a[2]);
  // printf("a3:%d\n",a[3]);
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

int fs_gettimeofday(struct timeval *tv, struct timezone *tz) {
  uint64_t uptime=0;
  ioe_read(AM_TIMER_UPTIME, &uptime);
  tv->tv_usec = (int32_t)uptime;
  tv->tv_sec = (int32_t)uptime / 1000000;
  return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  // printf("do syscall:%d\n",a[0]);
  switch (a[0]) {
    case SYS_exit:
      halt(0);
      break;
    case SYS_yield:
      yield();
      c->GPRx = 0; 
      break;
    case SYS_open:
      // yield();
      c->GPRx = fs_open((void *)a[1], a[2], a[3]);
      break;
    case SYS_read:
      c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
      break;
    case SYS_write:
      c->GPRx = fs_write(a[1], (void *)a[2], a[3]); 
      break;
    case SYS_close:
      c->GPRx = fs_close(a[1]);
      break;
    case SYS_lseek:
      c->GPRx = fs_lseek(a[1], a[2], a[3]);
      break;
    case SYS_brk:
      c->GPRx = 0; 
    break;
    case SYS_gettimeofday:
      c->GPRx = fs_gettimeofday((struct timeval *)a[1], (struct timezone *)a[2]); 
    break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
