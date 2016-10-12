#include "mem_pool.h"

#define MALLOC malloc
#define FREE free

#define MALLOC_DATA_FIX(_x_) \
    (as_mem_data_fixed_t *)MALLOC(sizeof(as_mem_data_fixed_t) + (_x_) - 1)


as_mem_pool_fixed_t *
mem_pool_fixed_new(size_t fsize[], unsigned n) {
  if (n < 1) {
    return NULL;
  }

  as_mem_pool_fixed_t *p;
  p = (as_mem_pool_fixed_t *)MALLOC(sizeof(as_mem_pool_fixed_t) +
                                  sizeof(as_mem_pool_fixed_field_t) * (n - 1));
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


static inline int
bin_search_position(as_mem_pool_fixed_t *p, size_t size) {
  int left = 0;
  int right = p->n - 1;
  int middle;
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


as_mem_data_fixed_t *
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
    d = MALLOC_DATA_FIX(size);
    if (d == NULL) {
      return NULL;
    }
    d->next = NULL;
    d->size = size;
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
      d->size = p->f[i].size;
    }
  }
  return d;
}


void
mem_pool_fixed_recycle(as_mem_pool_fixed_t *p, as_mem_data_fixed_t *d) {
  if (d != NULL && p == NULL) {
    FREE(d);
  }

  if (p != NULL && d != NULL) {
    int i = bin_search_position(p, d->size);
    if (p->f[i].size > d->size) {
      i -= 1;
    }
    if (i < 0) {
      FREE(d);
    } else {
      d->next = p->f[i].header;
      p->f[i].header = d;
      p->empty += p->f[i].size;
    }
  }
}


void
mem_pool_fixed_destroy(as_mem_pool_fixed_t *p) {
  if (p == NULL) {
    return;
  }

  int i = 0;
  for (i = 0; i < p->n; ++i) {
    while (p->f[i].header) {
      as_mem_data_fixed_t *d = p->f[i].header->next;
      FREE(p->f[i].header);
      p->f[i].header = d;
    }
  }
  FREE(p);
}
