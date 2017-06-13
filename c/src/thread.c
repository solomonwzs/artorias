#include "lua_bind.h"
#include "thread.h"


static as_tid_t cur_tid = 0;


int
asthread_init(as_thread_t *t, int fd, lua_State *L) {
  cur_tid += 1;
  lua_State *T = lbind_new_tid_lthread(L, cur_tid, fd);
  if (T == NULL) {
    cur_tid -= 1;
    return -1;
  }

  t->tid = cur_tid;
  t->T = T;
  t->ct = time(NULL);
  t->ut = t->ct;
  t->status = AS_TSTATUS_READY;
  t->fd = fd;
  t->resl = NULL;

  return cur_tid;
}
