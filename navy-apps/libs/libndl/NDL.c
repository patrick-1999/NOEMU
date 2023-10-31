#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

size_t dispinfo_read(void *buf, size_t offset, size_t len);


// static int evtdev = -1;
static int evtdev = 3;
static int fbdev = 5;
static int screen_w = 0, screen_h = 0;

typedef struct size
{
  int w;
  int h;
} Size;
Size disp_size;

uint32_t NDL_GetTicks() {
    // 以毫秒为单位返回系统时间
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  // printf("evtdev:%d\n",evtdev);
  buf[0]='\0';
  assert(evtdev!=-1);
  return read(evtdev, buf, len);
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
 if (*w == 0 && *h == 0)
  {
    *w = disp_size.w;
    *h = disp_size.h;
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if (w == 0 && h == 0)
  {
    w = disp_size.w;
    h = disp_size.h;
  }
  // printf("NDL_DrawRect, x=%d, y=%d, w=%d, h=%d\n", x, y, w, h);

  assert(w > 0 && w <= disp_size.w);
  assert(h > 0 && h <= disp_size.h);

  // w, h 为画布大小， 重新设置x，y的值以居中显示画布
  x = (disp_size.w - w) / 2;
  y = (disp_size.h - h) / 2;
  
  for (size_t row = 0; row < h; ++row)
  {
    lseek(fbdev, x + (y + row) * disp_size.w, SEEK_SET);
    write(fbdev, pixels + row * w, w);
  }
  write(fbdev, 0, 0);

}





void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  // FILE *fp = fopen("/proc/dispinfo", "w");
  // char * buf;
  // assert(fp != NULL);
  // fscanf(fp, "WIDTH:%d\nHEIGHT:%d\n", &disp_size.w, &disp_size.h);
  char * buf[20]={};
  dispinfo_read(buf,0,0);
  sscanf(buf,"WIDTH:%d\nHEIGHT:%d\n", &disp_size.w, &disp_size.h);

  printf("NDL_Init, disp_size.w=%d, disp_size.h=%d\n", disp_size.w, disp_size.h);
  assert(disp_size.w >= 400 && disp_size.w <= 800);
  assert(disp_size.h >= 300 && disp_size.h <= 640);
  // fclose(fp);
  return 0;
}

void NDL_Quit() {
}
