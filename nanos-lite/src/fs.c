#include <fs.h>
#include <stdio.h>
#include <stdlib.h>
#define NR_FILE (sizeof(file_table) / sizeof(file_table[0]))

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);


typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;
enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_DISINFO, FD_VGA};
// enum {SEEK_SET, SEEK_CUR, SEEK_END};
/* This is the information about all files in disk. */

size_t invalid_read(void *buf, size_t offset, size_t len);
size_t invalid_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);

static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, 0,invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0,invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, 0,invalid_read, serial_write},
  [FD_FB]     = {"/dev/fb", 0, 0, 0, events_read, invalid_write},//FrameBuffer和显示相关
  [FD_DISINFO] = {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
  [FD_VGA] = {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
#include "files.h"
};
int get_fs_len(){
  return sizeof(file_table)/sizeof(Finfo);
}

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

int fs_open(const char *pathname, int flags, int mode){
  // find file in file system by pathname
  for(int i=0;i<get_fs_len();i++){
    if(strcmp(pathname,file_table[i].name)==0){
      file_table[i].open_offset = 0;
      return i;
    }
    
  }
  printf("file not found!!!\n");
  assert(0);
  return -1;
}

int fs_close(int fd){
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(Finfo));
  return 0;
}
size_t fs_read(int fd, void *buf, size_t len){

assert(fd >= 0 && fd < sizeof(file_table) / sizeof(Finfo));

  Finfo *f = &file_table[fd];

  if (fd == FD_STDIN || fd == FD_FB ) {
    return f->read(buf, 0, len);
  }

  if (len == 0) return 0;
  if (f->open_offset >= f->size) return 0;

  if (len + f->open_offset > f->size) len = f->size - f->open_offset;

  size_t offset = f->disk_offset + f->open_offset;
  f->open_offset += len;
  if (f->read == NULL) {    // 普通文件，用ramdisk读
    return ramdisk_read(buf, offset, len);
  } else {
    return f->read(buf, offset, len);
  }
}
size_t fs_write(int fd, const void *buf, size_t len){
    assert(fd >= 0 && fd < sizeof(file_table) / sizeof(Finfo));

    Finfo *f = &file_table[fd];

    if (fd == FD_STDOUT || fd == FD_STDERR) {
        return f->write(buf, 0, len);
    }

    if (len == 0) return 0;
    if (f->open_offset >= f->size) return 0;

    if (len + f->open_offset > f->size) len = f->size - f->open_offset;

    size_t offset = f->disk_offset + f->open_offset;
    f->open_offset += len;
    if (f->write == NULL) {   // 普通文件，用ramdisk写
        return ramdisk_write(buf, offset, len);
    } else {
        return f->write(buf, offset, len);
    }
}
size_t fs_lseek(int fd, size_t offset, int whence){
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(Finfo));

  Finfo *f = &file_table[fd];

  switch (whence) {
    case SEEK_SET: f->open_offset = offset; break;
    case SEEK_CUR: f->open_offset += offset; break;
    case SEEK_END: f->open_offset = f->size + offset; break;
    default: assert(0);
  }
  return f->open_offset;
}






void init_fs() {
  // TODO: initialize the size of /dev/fb

  AM_GPU_CONFIG_T gpu_config;
  ioe_read(AM_GPU_CONFIG,&gpu_config);
  char buf [20]={};
  memset(buf,0,sizeof(buf));
  dispinfo_read(buf,0,0);
  int dispinfo = fs_open("/proc/dispinfo", 0, 0);
  printf("buf:%s\n",buf);
  fs_write(dispinfo, buf, sizeof(*buf));
  int width = gpu_config.width, height = gpu_config.height;
  int fb_fd = fs_open("/dev/fb", 0, 0);
  // 主要工作是改写文件表中fb_fd的文件大小
  file_table[fb_fd].size = width * height;
  printf("NR_FILE:%d\n",NR_FILE);
}


