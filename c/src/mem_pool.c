#include "mem_pool.h"

#define MALLOC malloc
#define FREE free

#define MALLOC_DATA_FIX(_x_) \
    (as_mem_data_fix_t *)MALLOC(sizeof(as_mem_data_fix_t) + (_x_) - 1)


as_mem_pool_fix_t *
mem_pool_fix_new(unsigned n, size_t fsize[]) {
  if (n < 1) {
    return NULL;
  }

  as_mem_pool_fix_t *p;
  p = (as_mem_pool_fix_t *)MALLOC(sizeof(as_mem_pool_fix_t) +
                                  sizeof(as_mem_pool_fix_field_t) * (n - 1));
  if (p == NULL) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n; ++i) {
    p->f[i].size = fsize[i];
    p->f[i].header = NULL;
  }
  p->n = n;
  p->used = 0;
  p->empty = 0;

  return p;
}

as_mem_data_fix_t *
mem_pool_fix_alloc(as_mem_pool_fix_t *p, size_t size) {
  if (p == NULL || size == 0) {
    return NULL;
  }

  int i = 0;
  int j = p->n - 1;
  int k;
  while (i < j) {
    k = (i + j) / 2;
    if (p->f[k].size == size) {
      i = k;
      break;
    } else if (p->f[k].size < size) {
      i = k + 1;
    } else {
      j = k - 1;
    }
  }
  if (p->f[i].size < size) {
    i += 1;
  }

  as_mem_data_fix_t *d;
  if (i >= p->n) {
    d = MALLOC_DATA_FIX(size);
    if (d == NULL) {
      return NULL;
    }
    d->next = NULL;
    d->idx = -1;
  } else {
    if (p->f[i].header != NULL) {
      d = p->f[i].header;
      p->f[i].header = d->next;
      d->next = NULL;
      p->empty -= p->f[i].size;
    } else {
      d = MALLOC_DATA_FIX(size);
      if (d == NULL) {
        return NULL;
      }
      d->next = NULL;
      d->idx = i;
    }
    p->used += p->f[i].size;
  }
  return d;
}

void
mem_pool_fix_free(as_mem_pool_fix_t *p, as_mem_data_fix_t *d) {
  if (d != NULL && (d->idx == -1 || p == NULL)) {
    free(d);
  }

  if (p != NULL && d != NULL) {
    int i = d->idx;
    d->next = p->f[i].header;
    p->f[i].header = d;
    p->empty += p->f[i].size;
  }
}

void
mem_pool_fix_destory(as_mem_pool_fix_t *p) {
  if (p == NULL) {
    return;
  }

  int i = 0;
  for (i = 0; i < p->n; ++i) {
    while (p->f[i].header) {
      as_mem_data_fix_t *d = p->f[i].header->next;
      FREE(p->f[i].header);
      p->f[i].header = d;
    }
  }
  FREE(p);
}
