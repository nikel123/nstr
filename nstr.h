/* Copyright (c) 2016 Andrej Gelenberg <andrej.gelenberg@udo.edu> */
#ifndef _NSTR_H
#define _NSTR_H

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdint.h>

#define nstr_likely(x)   __builtin_expect((x),1)
#define nstr_unlikely(x) __builtin_expect((x),0)

enum nstr_type_t {
  NSTR_EMPTY_T,
  NSTR_CONST_T,
  NSTR_CSTR_T,
  NSTR_BUFFER_T,
  NSTR_BUFFER_CSTR_T,
  NSTR_SUB_T,
  NSTR_LIST_T
};
typedef enum nstr_type_t nstr_type_t;

typedef struct nstr_t nstr_t;

struct nstr_t {
  nstr_type_t type;
  size_t      len;
  size_t      refs;

  union {

    struct {
      char   *str;
      char   *start;
      size_t len;
    } buf;

    struct {
      const char *str;
    } cstr;

    struct {
      nstr_t *ref;
      size_t offset;
    } sub;

    void **lst;

  };
};

static void
nstr_ref(
    nstr_t *nstr) {

  if ( nstr_likely(nstr->refs < SIZE_MAX) )
    ++(nstr->refs);

}

// only for use in nstr_unref
void
nstr_list_free(
    nstr_t *nstr);

static void
nstr_unref(
    nstr_t *nstr) {

  assert(nstr->refs);

  // we can't count past SIZE_MAX, so
  // we won't free it if is could be more than that
  if ( nstr_likely(nstr->refs < SIZE_MAX) )
    --(nstr->refs);

  if ( nstr->refs == 0 )  {
    switch(nstr->type) {

      case NSTR_LIST_T:
        nstr_list_free(nstr);
        break;

	  case NSTR_SUB_T:
	    nstr_unref(nstr->sub.ref);
		goto nstr_free;

      case NSTR_BUFFER_T:
      case NSTR_BUFFER_CSTR_T:
        free(nstr->buf.start);

      case NSTR_EMPTY_T:
      case NSTR_CONST_T:
      case NSTR_CSTR_T:
	  nstr_free:
        free(nstr);

    }
  }
}

int
nstr_compact(
    nstr_t *nstr);

const char *
nstr_cstr(
    nstr_t *nstr);

static nstr_t *
nstr_new()
    __attribute__((unused));

static nstr_t *
nstr_new() {

  nstr_t *ret = malloc(sizeof(*ret));

  if ( nstr_likely(ret != 0) ) {
    ret->type = NSTR_EMPTY_T;
    ret->len  = 0;
    ret->refs = 1;
  }

  return ret;

}

static nstr_t *
nstr_new_cstr_n(
    const char *str,
    size_t     len) {

  assert(str != 0);
  assert(len != 0);

  nstr_t *ret = malloc(sizeof(*ret));

  if ( nstr_likely(ret != 0) ) {
    ret->type     = NSTR_CSTR_T;
    ret->len      = len;
    ret->refs     = 1;
    ret->cstr.str = str;
  }

  return ret;

}

static nstr_t *
nstr_new_cstr(
    const char *str)
  __attribute__((unused));

static nstr_t *
nstr_new_cstr(
    const char *str) {

  assert(str != 0);
  return nstr_new_cstr_n(str, strlen(str));

}

static nstr_t *
nstr_new_const(
    const char *str,
    size_t      len)
  __attribute__((unused));

static nstr_t *
nstr_new_const(
    const char *str,
    size_t     len) {

  assert(str != 0);
  assert(len != 0);

  nstr_t *ret = malloc(sizeof(*ret));

  if ( nstr_likely(ret != 0) ) {
    ret->type     = NSTR_CONST_T;
    ret->len      = len;
    ret->refs     = 1;
    ret->cstr.str = str;
  }

  return ret;

}

static nstr_t *
nstr_new_buffer(
    char   *start,
    char   *str,
    size_t len,
    size_t buflen)
  __attribute__((unused));

static nstr_t *
nstr_new_buffer(
    char   *start,
    char   *str,
    size_t len,
    size_t buflen) {

  assert(start);
  assert(str >= start);
  assert(len);
  assert(buflen >= len);

  nstr_t *ret = malloc(sizeof(*ret));

  if ( nstr_likely(ret != 0) ) {
    if ( buflen - len > str - start ) {

      if ( str[len] != 0 )
        str[len] = 0;

      ret->type = NSTR_BUFFER_CSTR_T;

    } else {

      ret->type = NSTR_BUFFER_T;

    }

    ret->len       = len;
    ret->refs      = 1;
    ret->buf.str   = str;
    ret->buf.start = start;
    ret->buf.len   = buflen;
  }

  return ret;

}

static nstr_t *
nstr_new_sub(
    nstr_t *ref,
    size_t offset,
    size_t len)
  __attribute__((unused));

static nstr_t *
nstr_new_sub(
    nstr_t *ref,
    size_t offset,
    size_t len) {

  nstr_t *ret;

  assert(ref != 0);

  // check for int overflow
  if ( nstr_unlikely(
      ((SIZE_MAX - offset) < len) ||
      (ref->len < (offset + len)) )) {

    ret = 0;
    goto end;

  }

  ret = malloc(sizeof(*ret));

  if ( nstr_likely(ret != 0) ) {

    if ( ref->type == NSTR_EMPTY_T ) {

      assert(offset == 0);
      assert(len    == 0);

      ret->type = NSTR_EMPTY_T;
      ret->len  = 0;

    } else {

      assert(len != 0);

      ret->type = NSTR_SUB_T;
      ret->len  = len;

      nstr_ref(ref);
      ret->sub.ref    = ref;
      ret->sub.offset = offset;

    }

    ret->refs = 1;

  }

end:
  return ret;

}

void
nstr_copy_to_buf(
    char   *target,
    nstr_t *nstr,
    size_t offset,
    size_t len);

/*
  concatenate strings and return resulting string.
  argument list must be terminated by NULL (0).
  source strings will be unreferenced
*/
nstr_t *
nstr_concat(
    nstr_t *a,
    ...);

#endif // _NSTR_H
