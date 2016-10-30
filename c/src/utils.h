#ifndef __AS_UTILS__
#define __AS_UTILS__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define as_malloc malloc
#define as_free free

#define DEBUG
#ifdef DEBUG
#   define debug_log(_fmt_, ...) \
    fprintf(stderr, "\033[0;33m=%d= [%s:%d:%s]\033[0m " _fmt_, getpid(), \
            __FILE__, __LINE__, __func__, ## __VA_ARGS__)
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
#     define offsetof(st, m) __builtin_offsetof(st, m)
#   else
#     define offsetof(st, m) ((size_t)&(((st *)0)->m))
#   endif
#endif

#define container_of(ptr, type, member) \
({\
  const typeof(((type *)0)->member)*__mptr = (ptr);\
  (type *)((char *)__mptr - offsetof(type, member));\
 })

#endif

#define binary_search(arr, len, key, val) \
({\
  typeof(len) __left = 0;\
  typeof(len) __right = (len) - 1;\
  typeof(len) __mid;\
  while (__left < __right) {\
    __mid = (__left + __right) / 2;\
    if (val((arr), __mid) == key) {\
      __left = __mid;\
    break;\
    } else if (val((arr), __mid) < key) {\
      __left = __mid + 1;\
    } else {\
      __right = __mid - 1;\
    }\
  }\
  __left;\
 })
