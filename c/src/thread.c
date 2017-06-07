#include "thread.h"


static as_tid_t cur_tid = 0;


void
asthread_init(as_thread_t *t, as_thread_pool_t *pool, int fd, lua_State *L) {
  cur_tid += 1;

  t->tid = cur_tid;
  t->ct = time(NULL);
  t->ut = t->ct;
  t->status = AS_TSTATUS_OK;
  t->fd = fd;
}
