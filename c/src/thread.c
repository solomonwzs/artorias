#include "lua_bind.h"
#include "thread.h"


#define node_et_lt(a, b) (container_of(a, as_thread_t, p_idx)->et < \
                          container_of(b, as_thread_t, p_idx)->et)


static as_tid_t cur_tid = 0;


int
asthread_init(as_thread_t *th, lua_State *L) {
  cur_tid += 1;
  lua_State *T = lbind_new_tid_lthread(L, cur_tid);
  if (T == NULL) {
    cur_tid -= 1;
    return -1;
  }

  th->tid = cur_tid;
  th->T = T;
  th->ct = time(NULL);
  th->ut = th->ct;
  th->status = AS_TSTATUS_READY;
  th->resl = NULL;

  return cur_tid;
}


void
asthread_pool_insert(as_thread_t *th) {
  as_rb_tree_t *pool = th->pool;

  rb_tree_insert(pool, &th->p_idx, node_et_lt);
  rb_tree_insert_case(pool, &th->p_idx);
}


void
asthread_pool_delete(as_thread_t *th) {
  as_rb_tree_t *pool = th->pool;

  rb_tree_delete(pool, &th->p_idx);
}


void
asthread_array_add(as_thread_array_t *array, as_thread_t *th) {
  *(array->ths + array->n) = th;
  array->n += 1;
}
