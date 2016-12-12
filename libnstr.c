#include <nstr.h>
#include <stdarg.h>

enum {
  nstr_list_order = 1 // should be odd
};

struct nstr_list_iterator_t {
  size_t i;
  void **lst;
};
typedef struct nstr_list_iterator_t nstr_list_iterator_t;

static void
nstr_list_iterator_init(
    nstr_list_iterator_t *iterator,
    nstr_t *nstr) {

  assert(nstr);
  assert(nstr->type == NSTR_LIST_T);
  assert(nstr->len != 0);

  iterator->i   = 0;
  iterator->lst = nstr->lst;
}

static void **
nstr_list_new_lst() {
  return calloc(nstr_list_order + 1, sizeof(void *));
}

static nstr_t *
nstr_list_iterator_next(
    nstr_list_iterator_t *iterator) {

  assert(iterator->lst != 0);

  size_t i = iterator->i;
  nstr_t *ret = iterator->lst[i];

  if ( i < nstr_list_order ) {
    ++(iterator->i);
  } else {
    iterator->lst = iterator->lst[i];
    iterator->i   = 0;
  }

  return ret;
}

static void
nstr_list_copy_to_buf(
    char   *target,
    nstr_t *nstr,
    size_t offset,
    size_t len) {

  assert(target);
  assert(nstr);
  assert(nstr->type == NSTR_LIST_T);

  nstr_list_iterator_t itr;
  nstr_t *cur;
  size_t rest_len;

  nstr_list_iterator_init(&itr, nstr);

  // skip offset

  while(1) {

    cur = nstr_list_iterator_next(&itr);

    // it should not happen, what we
    // run out of substrings before
    // we skiped offset
    // otherwise it would mean that
    // the data structure is inconsistent
    assert(cur != 0);

    if ( cur->len > offset ) break;
    else offset -= cur->len;

  }

  rest_len = cur->len - offset;
  if ( rest_len > len ) rest_len = len;

  nstr_copy_to_buf(
      target,
      cur,
      offset,
      rest_len);

  len -= rest_len;
  if ( len == 0 ) goto end;
  target += rest_len;

  while(1) {

    cur = nstr_list_iterator_next(&itr);

    // it should not run out of substrings
    // before we copied all required data
    // otherwise it would mean that the
    // data structure is inconsistent
    assert(cur != 0);

    rest_len = (cur->len < len) ? cur->len : len;

    nstr_copy_to_buf(
        target,
        cur,
        0,
        rest_len);

    len -= rest_len;
    if ( len == 0 ) goto end;
    target += rest_len;

  }

end:
  return;
}

void
nstr_copy_to_buf(
    char   *target,
    nstr_t *nstr,
    size_t offset,
    size_t len) {

  if ( len == 0 )
    return;

  assert(target);
  assert(nstr);
  assert(nstr->len >= (offset + len));

  switch(nstr->type) {

    case NSTR_CONST_T:
    case NSTR_CSTR_T:
    case NSTR_BUFFER_T:
    case NSTR_BUFFER_CSTR_T:
      memcpy(target, nstr->cstr.str + offset, len);
      break;

    case NSTR_SUB_T:
      nstr_copy_to_buf(
          target,
          nstr->sub.ref,
          offset + nstr->sub.offset,
          len);
      break;

    case NSTR_LIST_T:
      nstr_list_copy_to_buf(
          target,
          nstr,
          offset,
          len);
      break;

    case NSTR_EMPTY_T:
      break;

  }
}

static int
nstr_sub_compact(
    nstr_t *nstr) {

  char *buf;
  int  ret;

  assert(nstr != 0);
  assert(nstr->type == NSTR_SUB_T);
  assert(nstr->sub.ref->type != NSTR_EMPTY_T);

  // check for int overflow
  if ( nstr_unlikely(nstr->len >= SIZE_MAX) )
    goto err;

  // alloc buffer for the compacted nstr
  const size_t buflen = nstr->len + 1;
  buf = malloc(buflen);
  if ( nstr_likely(buf == 0) )
    goto err;

  // copy content to the new buffer
  nstr_copy_to_buf(
      buf,
      nstr->sub.ref,
      nstr->sub.offset,
      nstr->len);

  buf[nstr->len] = 0;

  // don't need old ref
  nstr_unref(nstr->sub.ref);

  // adjust nstr
  nstr->type      = NSTR_BUFFER_CSTR_T;
  nstr->buf.str   = buf;
  nstr->buf.start = buf;
  nstr->buf.len   = buflen;

  ret = 0;

end:
  return ret;

err:
  ret = 1;
  goto end;
}

static int
nstr_list_compact(
    nstr_t *nstr) {

  int ret;

  assert(nstr != 0);
  assert(nstr->type == NSTR_LIST_T);

  const size_t buflen = nstr->len + 1;
  char *buf = malloc(buflen);

  if (nstr_unlikely(buf == 0))
    goto err;

  char *bufi = buf;

  void **lst = nstr->lst;
  void **next;
  size_t i;
  nstr_t *subnstr;

  while(lst != 0) {
    for(i = 0; i < nstr_list_order; ++i) {
      subnstr = lst[i];

      if ( nstr_unlikely(subnstr == 0) ) {
        free(lst);
        goto end_loop;
      }

      nstr_copy_to_buf(
          bufi,
          subnstr,
          0,
          subnstr->len);
      bufi += subnstr->len;
      nstr_unref(subnstr);
    }

    next = lst[i];
    free(lst);
    lst = next;
  }

end_loop:
  buf[nstr->len] = 0;
  nstr->type = NSTR_BUFFER_CSTR_T;
  nstr->buf.str = nstr->buf.start = buf;
  nstr->buf.len = buflen;

  ret = 0;
end:
  return ret;

err:
  ret = 1;
  goto end;
}

/* campact the nstr
 (returns NSTR_BUFFER_CSTR_T)

 if returns not 0, then don't use nstr argument anymore
 can be already freed
 if return 0, then nstr should stay unmodified
*/
int
nstr_compact(
    nstr_t *nstr) {

  int ret;

  switch(nstr->type) {
    case NSTR_EMPTY_T:
    case NSTR_CONST_T:
    case NSTR_CSTR_T:
    case NSTR_BUFFER_T:
    case NSTR_BUFFER_CSTR_T:
      ret = 0;
      break;

    case NSTR_SUB_T:
      ret = nstr_sub_compact(nstr);
      break;

    case NSTR_LIST_T:
      ret = nstr_list_compact(nstr);
      break;
  }

  return ret;
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
    nstr->buf.str = nstr->buf.start = ret;
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
    // there is no space to convert
    // NSTR_BUFFER_T to NSTR_BUFFER_CSTR_T

    assert(nstr->buf.start == nstr->buf.str);
    ret = realloc(nstr->buf.str, nstr->len + 1);

    if ( nstr_likely(ret != 0) ) {
      nstr->type     = NSTR_BUFFER_CSTR_T;
      nstr->buf.len  = nstr->len + 1;
      nstr->buf.str  = nstr->buf.start = ret;
      ret[nstr->len] = 0;
    }

  } else {
    // there is space to make inline modifications
    // so convert NSTR_BUFFER_T to NSTR_BUFFER_CSTR_T

    nstr->type = NSTR_BUFFER_CSTR_T;
    if ( nstr->buf.len - nstr->len >
           nstr->buf.str - nstr->buf.start ) {
      ret = nstr->buf.str;
    } else {
      ret = nstr->buf.start;
      memmove(ret, nstr->buf.str, nstr->len);
      nstr->buf.str = ret;
    }

    if ( ret[nstr->len] != 0 ) {
      ret[nstr->len] = 0;
    }

  }

  return ret;
}

static const char *
nstr_sub_cstr(
    nstr_t *nstr) {

  const char *ret;

  assert(nstr != 0);
  assert(nstr->type == NSTR_SUB_T);
  assert(nstr->sub.ref->type != NSTR_EMPTY_T);

  nstr_t *sub = nstr->sub.ref;
  const size_t offset = nstr->sub.offset;

  switch(sub->type) {
    case NSTR_CSTR_T:
    case NSTR_BUFFER_CSTR_T:
      if ( (offset + nstr->len) == sub->len ) {
        ret = sub->cstr.str + offset;
        break;
      }

    default:
      if ( nstr_unlikely(nstr_sub_compact(nstr)) ) {

        ret = 0;

      } else {

        assert(nstr->type == NSTR_BUFFER_CSTR_T);
        ret = nstr->cstr.str;

      }
  }

  return ret;
}

const char *
nstr_cstr(
    nstr_t *nstr) {

  const char *ret;

  assert(nstr->refs);

  switch(nstr->type) {

    case NSTR_CSTR_T:
    case NSTR_BUFFER_CSTR_T:
      ret = nstr->cstr.str;
      break;

    case NSTR_CONST_T:
      ret = nstr_const_cstr(nstr);
      break;

    case NSTR_BUFFER_T:
      ret = nstr_buffer_cstr(nstr);
      break;

    case NSTR_SUB_T:
      ret = nstr_sub_cstr(nstr);
      break;

    case NSTR_LIST_T:
      if ( nstr_unlikely(nstr_list_compact(nstr)) ) {
        ret = 0;
      } else {
        assert(nstr->type == NSTR_BUFFER_CSTR_T);
        ret = nstr->cstr.str;
      }
      break;

    case NSTR_EMPTY_T:
      ret = "";
      break;

  }

  return ret;
}

nstr_t *
nstr_concat(
    nstr_t *instr,
    ...) {

  va_list ap;
  va_start(ap, instr);

  nstr_t *ret;
  void **lst;
  void **next;
  size_t i;

  // skip empty strings at the begining
  while(1) {
    if ( instr == 0 )
      goto err_unref_args;
    if ( instr->len != 0 )
      break;
    nstr_unref(instr);
    instr = va_arg(ap, nstr_t *);
  }

  // create new list
  ret = malloc(sizeof(*ret));
  if ( nstr_unlikely(ret == 0) )
    goto err_unref_args;

  lst = nstr_list_new_lst();
  if ( nstr_unlikely(lst == 0) )
    goto err_new_lst;

  ret->type = NSTR_LIST_T;
  ret->len  = 0;
  ret->refs = 1;
  ret->lst  = lst;
  i = 0;

  // append to the list
  while(instr !=0) {
    // skip empty strings
    if ( nstr_unlikely(instr->len == 0) ) {
      nstr_unref(instr);
      continue;
    }

    // extend list if needed
    if ( nstr_unlikely(i >= nstr_list_order) ) {
      next = nstr_list_new_lst();
      if ( nstr_unlikely(next == 0) )
        goto err_lst_alloc;
      lst[i] = next;
      lst = next;
      i = 0;
    }

    ret->len += instr->len;
    lst[i]   = instr;
    instr    = va_arg(ap, nstr_t *);
    ++i;
  }

end:
  va_end(ap);
  return ret;

err_lst_alloc:
  nstr_unref(ret);
  goto err_unref_args;

err_new_lst:
  free(ret);

err_unref_args:
  // clean up args in error case
  while(instr != 0) {
    nstr_unref(instr);
    instr = va_arg(ap, nstr_t *);
  }

  ret = 0;
  goto end;
}

void
nstr_list_free(
    nstr_t *nstr) {

  assert(nstr);
  assert(nstr->type == NSTR_LIST_T);

  nstr_t *cur;
  void   **lst;
  void   **next;
  size_t i;

  lst = nstr->lst;

  assert(lst != 0);

  while(lst != 0) {

    for(i=0; i < nstr_list_order; ++i) {

      cur = lst[i];

      if ( cur != 0 ) {
        nstr_unref(cur);
      } else {
        free(lst);
        goto end;
      }

    }

    next = lst[i];
    free(lst);
    lst = next;

  }

end:
  return;
}
