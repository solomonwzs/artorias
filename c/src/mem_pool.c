#include "mem_pool.h"

#define malloc_data_fixed(_x_) \
    (as_mem_data_fixed_t *)as_malloc(offsetof(as_mem_data_fixed_t, d) + (_x_))


as_mem_pool_fixed_t *
mem_pool_fixed_new(size_t fsize[], unsigned n) {
  if (n < 1) {
    return NULL;
  }

  as_mem_pool_fixed_t *p;
  p = (as_mem_pool_fixed_t *)as_malloc(
      sizeof(as_mem_pool_fixed_t) +
      sizeof(as_mem_pool_fixed_field_t) * (n - 1));
  if (p == NULL) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n; ++i) {
    p->f[i].size = fsize[i];
    p->f[i].header = NULL;
    p->f[i].pool = p;
  }
  p->n = n;
  p->used = 0;
  p->empty = 0;

  return p;
}


static inline uint8_t
bin_search_position(as_mem_pool_fixed_t *p, size_t size) {
  uint8_t left = 0;
  uint8_t right = p->n - 1;
  uint8_t middle;
  while (left < right) {
    middle = (left + right) / 2;
    if (p->f[middle].size == size) {
      left = middle;
      break;
    } else if (p->f[middle].size < size) {
      left = middle + 1;
    } else {
      right = middle - 1;
    }
  }
  return left;
}


void *
mem_pool_fixed_alloc(as_mem_pool_fixed_t *p, size_t size) {
  if (p == NULL || size == 0) {
    return NULL;
  }

  int i = bin_search_position(p, size);
  if (p->f[i].size < size) {
    i += 1;
  }
  as_mem_data_fixed_t *d;
  if (i >= p->n) {
    d = malloc_data_fixed(size);
    if (d == NULL) {
      return NULL;
    }
    d->p = NULL;
  } else {
    if (p->f[i].header != NULL) {
      d = p->f[i].header;
      p->f[i].header = d->p;
      d->p = p;
      p->empty -= p->f[i].size;
    } else {
      d = malloc_data_fixed(p->f[i].size);
      if (d == NULL) {
        return NULL;
      }
      d->p = &p->f[i];
    }
  }
  return (void *)d->d;
}


void
mem_pool_fixed_recycle(void *dd) {
  if (dd == NULL) {
    return;
  }

  as_mem_data_fixed_t *d = container_of(dd, as_mem_data_fixed_t, d);
  as_mem_pool_fixed_field_t *f = (as_mem_pool_fixed_field_t *)d->p;
  if (f == NULL) {
    as_free(d);
  }

  as_mem_pool_fixed_t *pool = f->pool;
  d->p = f->header;
  f->header = d;
  pool->empty += f->size;
}


void
mem_pool_fixed_destroy(as_mem_pool_fixed_t *p) {
  if (p == NULL) {
    return;
  }

  uint8_t i = 0;
  for (i = 0; i < p->n; ++i) {
    while (p->f[i].header) {
      as_mem_data_fixed_t *d = p->f[i].header->p;
      as_free(p->f[i].header);
      p->f[i].header = d;
    }
  }
  as_free(p);
}
