#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <NDL.h>
// #include "../../libs/libndl/include/NDL.h"
#include <BMP.h>
// #include "../../libs/libbmp/include/BMP.h"

int main() {
  NDL_Init(0);
  int w, h;
  void *bmp = BMP_Load("/share/pictures/projectn.bmp", &w, &h);
  printf("file opened\n");
  assert(bmp);
  NDL_OpenCanvas(&w, &h);
  printf("1\n");
  NDL_DrawRect(bmp, 0, 0, w, h);
  printf("2\n");
  free(bmp);
  NDL_Quit();
  printf("Test ends! Spinning...\n");
  while (1);
  return 0;
}
