#include <am.h>
#include <nemu.h>
#include <stdio.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  // kbd->keydown = 0;
  // kbd->keycode = AM_KEY_NONE;
  uint32_t kc = inl(KBD_ADDR);
  kbd->keydown = kc & KEYDOWN_MASK ;
  kbd->keycode = kc & ~KEYDOWN_MASK;
  printf("kbd->keydown:%d\n",kbd->keydown);
  printf("kbd->keycode:%d\n",kbd->keycode);
}
