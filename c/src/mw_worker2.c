#include "mw_worker.h"
#include "mem_pool.h"
#include <signal.h>
#include <stdint.h>
#include <sys/epoll.h>
#include "lua_bind.h"
#include "rb_tree.h"
#include "server.h"

#define AS_EPTR_TYPE_CFD      0x00
#define AS_EPTR_TYPE_THREAD   0x01
#define AS_EPTR_TYPE_TRES     0x02

typedef struct {
  int     type;
  uint8_t d[];
} as_epoll_event_ptr_t;


#define sizeof_event_ptr(_type_) \
    (offsetof(as_epoll_event_ptr_t, d) + sizeof(_type_))

#define get_cnf_int_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.i;\
})

#define get_cnf_str_val(_cnf_, _n_, ...) ({\
  as_cnf_return_t __ret = lpconf_get_pconf_value(_cnf_, _n_, ## __VA_ARGS__);\
  __ret.val.s;\
})


static volatile int keep_running = 1;
static void
init_handler(int dummy) {
  keep_running = 0;
}


static void
process_io(int epfd) {
  struct epoll_event events[100];
  int active_cnt = epoll_wait(epfd, events, 100, 500);
  for (int i = 0; i < active_cnt; ++i) {
    as_epoll_event_ptr_t *eptr = events[i].data.ptr;
    if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
        !(events[i].events & EPOLLIN || events[i].events & EPOLLOUT)) {
    } else if (events[i].data.ptr) {
    }
  }
}


static void
process(int cfd, as_lua_pconf_t *cnf, int single_mode) {
  set_non_block(cfd);
  if (single_mode) {
    signal(SIGINT, init_handler);
  }

  size_t fs[] = {8, 12, 16, 24, 32, 48, 64, 128, 256, 384, 512, 768, 1064};
  as_mem_pool_fixed_t *mem_pool = mpf_new(fs, sizeof(fs) / sizeof(fs[0]));

  as_rb_tree_t io_pool = {NULL};
  as_rb_tree_t sleep_pool = {NULL};
  as_rb_tree_t cio_pool = {NULL};

  int epfd = epoll_create(1);
  struct epoll_event event;

  lua_State *L = lbind_new_state(mem_pool);
  int conn_timeout = get_cnf_int_val(cnf, 1, "conn_timeout");
  const char *lfile = get_cnf_str_val(cnf, 1, "worker");

  as_epoll_event_ptr_t *cfd_ptr = mpf_alloc(mem_pool, sizeof_event_ptr(int));
  cfd_ptr->type = AS_EPTR_TYPE_CFD;
  *((int *)cfd_ptr->d) = cfd;

  event.data.ptr = cfd_ptr;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);

  lbind_init_state(L, mem_pool);
  lbind_append_lua_cpath(L, get_cnf_str_val(cnf, 1, "lua_cpath"));
  lbind_append_lua_path(L, get_cnf_str_val(cnf, 1, "lua_path"));
  lbind_reg_integer_value(L, LRK_SERVER_EPFD, epfd);
  lbind_ref_lcode_chunk(L, lfile);

  while (keep_running) {
  }
}


void
worker_process2(int channel_fd, as_lua_pconf_t *cnf) {
  process(channel_fd, cnf, 0);
}


void
test_worker_process2(int fd, as_lua_pconf_t *cnf) {
  process(fd, cnf, 1);
}
