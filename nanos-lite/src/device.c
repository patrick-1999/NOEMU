#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; i ++) {
    if (((char *)buf)[i] == 0) { return i; }
    putch(((char *)buf)[i]);
  }
  return len;
}
static AM_INPUT_KEYBRD_T ev;
char *strncat(char *dst, const char *src, size_t n);

size_t events_read(void *buf, size_t offset, size_t len) {
  ioe_read(AM_INPUT_KEYBRD, &ev);
  // if (ev.keycode != AM_KEY_NONE) {
  //   Log("keycode = %s, keydown = %d", keyname[ev.keycode], ev.keydown);
  // }
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  }
  if (ev.keydown) {
    strncat(buf, "kd ", 3);
  } else {
    strncat(buf, "ku ", 3);
  }
  strncat(buf, keyname[ev.keycode], len - 3);
  strncat(buf, "\n", len - 3 - strlen(keyname[ev.keycode]));
  return strlen(buf);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
