#ifndef __AS_UTILS__
#define __AS_UTILS__

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define as_malloc   malloc
#define as_calloc   calloc
#define as_realloc  realloc
#define as_free     free

#define dlog(_fmt_, ...) \
    fprintf(stderr, "\033[0;33m=%d= [%s:%d:%s]\033[0m " _fmt_, getpid(), \
            __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define DEBUG
#ifdef DEBUG
#   define debug_log dlog
#   define debug_perror(_s_) debug_log("%s: %s\n", _s_, strerror(errno))
#else
#   define debug_log(_fmt_, ...)
#   define debug_perror(_s_)
/*
#   define debug_perror(_s_) perror(_s_)
#   define _debug_log(_fmt_, ...) \
    fprintf(stderr, "\033[0;33m[%s:%d:%s] %d\033[0m " _fmt_, __FILE__, \
            __LINE__, __func__, getpid(), ## __VA_ARGS__)
#   define debug_perror(_s_) _debug_log("%s: %s\n", _s_, strerror(errno))
*/
#endif

#ifndef offsetof
#   ifdef __GNUC__
#     define offsetof(_st_, _m_) __builtin_offsetof(_st_, _m_)
#   else
#     define offsetof(_st_, _m_) ((size_t)&(((_st_ *)0)->_m_))
#   endif
#endif

#define container_of(_ptr_, _type_, _member_) ({\
  const typeof(((_type_ *)0)->_member_)*__mptr = (_ptr_);\
  (_type_ *)((char *)__mptr - offsetof(_type_, _member_));\
})

#define binary_search(_arr_, _len_, _key_, _eq_, _lt_) ({\
  typeof(_len_) __left = 0;\
  typeof(_len_) __right = (_len_) - 1;\
  typeof(_len_) __mid;\
  while (__left < __right) {\
    __mid = (__left + __right) / 2;\
    if (_eq_((_arr_), __mid, (_key_))) {\
      __left = __mid;\
    break;\
    } else if (_lt_((_arr_), __mid, (_key_))) {\
      __left = __mid + 1;\
    } else {\
      __right = __mid - 1;\
    }\
  }\
  __left;\
})

typedef uint32_t as_tid_t;

#endif
