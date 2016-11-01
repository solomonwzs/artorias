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

  int i;
  int j;
  for (i = 0; i < n; ++i) {
    p->f[i].size = fsize[i];
    p->f[i].header = NULL;
    p->f[i].pool = p;
  }

  size_t tmp;
  for (i = 0; i < n - 1; ++i) {
    for (j = i + 1; j < n; ++j) {
      if (p->f[i].size > p->f[j].size) {
        tmp = p->f[i].size;
        p->f[i].size = p->f[j].size;
        p->f[j].size = tmp;
      }
    }
  }

  p->n = n;
  p->empty = 0;

  return p;
}


#define field_size_eq(f, idx, s) ((f)[idx].size == (s))
#define field_size_lt(f, idx, s) ((f)[idx].size < (s))

void *
mem_pool_fixed_alloc(as_mem_pool_fixed_t *p, size_t size) {
  if (p == NULL || size == 0) {
    return NULL;
  }

  int i = binary_search(p->f, p->n, size, field_size_eq, field_size_lt);
  if (p->f[i].size < size) {
    i += 1;
  }
  as_mem_data_fixed_t *d;
  if (i >= p->n) {
    d = malloc_data_fixed(size);
    if (d == NULL) {
      return NULL;
    }
    d->p.field = NULL;
  } else {
    if (p->f[i].header != NULL) {
      d = p->f[i].header;
      p->f[i].header = d->p.next;
      d->p.field = &p->f[i];
      p->empty -= p->f[i].size;
    } else {
      d = malloc_data_fixed(p->f[i].size);
      if (d == NULL) {
        return NULL;
      }
      d->p.field = &p->f[i];
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
  as_mem_pool_fixed_field_t *f = (as_mem_pool_fixed_field_t *)d->p.field;
  if (f == NULL) {
    as_free(d);
    return;
  }

  as_mem_pool_fixed_t *pool = f->pool;
  d->p.next = f->header;
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
      as_mem_data_fixed_t *d = p->f[i].header->p.next;
      as_free(p->f[i].header);
      p->f[i].header = d;
    }
  }
  as_free(p);
}
