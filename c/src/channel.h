#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <unistd.h>

typedef struct {
  pid_t pid;
  int fd;
} as_channel_t;

extern void t();

#endif
