#ifndef __REDIS_PARSER__
#define __REDIS_PARSER__

typedef struct as_bytes_buffer_data_s {
  struct as_bytes_buffer_data_s   *next;
  size_t                          size;
  size_t                          used;
  char                            buf[1];
} as_bytes_buffer_data_t;

typedef struct {
  as_bytes_buffer_data_t  *header;
  as_bytes_buffer_data_t  *cur;
  unsigned                n;
  unsigned                len;
} as_bytes_buffer_t;

#endif
