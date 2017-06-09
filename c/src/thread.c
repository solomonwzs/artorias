#include "thread.h"


static as_tid_t cur_tid = 0;


void
asthread_init(as_thread_t *t, as_rb_tree_t *pool, int fd, lua_State *T) {
  cur_tid += 1;

  t->tid = cur_tid;
  t->T = T;
  t->ct = time(NULL);
  t->ut = t->ct;
  t->pool = NULL;
  t->status = AS_TSTATUS_READY;
  t->fd = fd;
  t->resl = NULL;
}
