/* Copyright (c) 2016 Andrej Gelenberg <andrej.gelenberg@udo.edu> */
#ifndef _NSTR_H
#define _NSTR_H

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>
#include <stdlib.h>

#define nstr_likely(x)   __builtin_expect((x),1)
#define nstr_unlikely(x) __builtin_expect((x),0)

enum nstr_type_t {
  NSTR_CONST_T,
  NSTR_CSTR_T,
  NSTR_BUFFER_T,
  NSTR_BUFFER_CSTR_T,
};
typedef enum nstr_type_t nstr_type_t;

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

  };
};
typedef struct nstr_t nstr_t;

static void
nstr_ref(
    nstr_t *nstr)
  __attribute__((unused));

static void
nstr_ref(
    nstr_t *nstr) {

  assert(nstr->refs);
  ++(nstr->refs);

}

static void
nstr_unref(
    nstr_t *nstr)
  __attribute__((unused));

static void
nstr_unref(
    nstr_t *nstr) {

  assert(nstr->refs);
  --(nstr->refs);

  if ( nstr->refs == 0 )  {
    switch(nstr->type) {
      case NSTR_BUFFER_T:
      case NSTR_BUFFER_CSTR_T:
        free(nstr->buf.start);

      case NSTR_CONST_T:
      case NSTR_CSTR_T:
        free(nstr);
    }
  }
}

static const char *
nstr_const_cstr(
    nstr_t *nstr) {

  assert(nstr->refs);
  assert(nstr->len);
  assert(nstr->cstr.str);

  char *ret = malloc(nstr->len + 1);

  if ( nstr_likely(ret != 0) ) {
    memcpy(ret, nstr->cstr.str, nstr->len);
    ret[nstr->len] = 0;

    nstr->type    = NSTR_BUFFER_CSTR_T;
    nstr->buf.len = nstr->len + 1;
    nstr->buf.str =
      nstr->buf.start = ret;
  }

  return ret;

}

static const char *
nstr_buffer_cstr(
    nstr_t *nstr) {

  assert(nstr->refs);
  assert(nstr->len);
  assert(nstr->buf.len >= nstr->len);
  assert(nstr->buf.start);
  assert(nstr->buf.str >= nstr->buf.start);

  char *ret;

  if ( nstr->buf.len <= nstr->len ) {

    assert(nstr->buf.start == nstr->buf.str);
    ret = realloc(nstr->buf.str, nstr->len + 1);

    if ( nstr_likely(ret != 0) ) {
      nstr->type    = NSTR_BUFFER_CSTR_T;
      nstr->buf.len = nstr->len + 1;
      nstr->buf.str =
        nstr->buf.start = ret;
      ret[nstr->len] = 0;
    }

  } else {

    nstr->type = NSTR_BUFFER_CSTR_T;
    if ( nstr->buf.len - nstr->len >
           nstr->buf.str - nstr->buf.start ) {
      ret = nstr->buf.str;
    } else {
      ret = nstr->buf.start;
      memmove(ret, nstr->buf.str, nstr->len);
      nstr->buf.str = ret;
    }
    if ( ret[nstr->len] != 0 )
      ret[nstr->len] = 0;

  }

  return ret;

}

static const char *
nstr_cstr(
    nstr_t *nstr)
  __attribute__((unused));

static const char *
nstr_cstr(
    nstr_t *nstr) {

  const char *ret;

  assert(nstr->refs);

  switch(nstr->type) {

    case NSTR_CONST_T:
      ret = nstr_const_cstr(nstr);
      break;

    case NSTR_BUFFER_T:
      ret = nstr_buffer_cstr(nstr);
      break;

    case NSTR_CSTR_T:
    case NSTR_BUFFER_CSTR_T:
      ret = nstr->cstr.str;

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
    ret->len   = len;
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
    ret->len   = len;
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

#endif // _NSTR_H
