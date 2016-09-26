#ifndef __SELECT_SERVER_H__
#define __SELECT_SERVER_H__

#include <sys/select.h>
#include "server.h"

extern void
select_server(int fd);

#endif
