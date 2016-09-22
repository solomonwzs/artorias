#ifndef __WRAP_EVENT_H__
#define __WRAP_EVENT_H__

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>

#define BUFFER_LEN 8196

extern int
make_socket(uint16_t port);

#endif
