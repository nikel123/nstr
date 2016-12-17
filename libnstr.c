#include <nstr.h>
#include <stdarg.h>

#define if_unlikely(cond) if ( nstr_unlikely(cond) )
#define ldie_if(label, cond) if_unlikely(cond) goto label
#define die_if(cond) ldie_if(err, cond)

enum {
  nstr_list_order = 7 // should be odd
};

struct nstr_list_iterator_t {
  size_t i;
  void   **lst;
  nstr_t *nstr;
};
typedef struct nstr_list_iterator_t nstr_list_iterator_t;

static void
nstr_list_iterator_init(
    nstr_list_iterator_t *iterator,
    nstr_t *nstr) {

  assert(nstr);
  assert(nstr->type == NSTR_LIST_T);

  iterator->i    = 0;
  iterator->lst  = nstr->lst;
  iterator->nstr = nstr;
}

static void **
nstr_list_new_lst() {
  return calloc(nstr_list_order + 1, sizeof(void *));
}

static nstr_t *
nstr_list_iterator_next(
    nstr_list_iterator_t *iterator) {

  assert(iterator->lst != 0);

  size_t i    = iterator->i;
  nstr_t *ret = iterator->lst[i];

  if_unlikely(ret == 0)
    goto end;

  if ( i < nstr_list_order ) {
    ++(iterator->i);
  } else {
    iterator->lst = iterator->lst[i];
    iterator->i   = 0;
  }

end:
  return ret;
}

static int
nstr_list_iterator_append(
    nstr_list_iterator_t *iterator,
    nstr_t               *nstr) {

  int ret;

  assert(nstr);
  assert(iterator);
  assert(iterator->lst);
  assert(iterator->i <= nstr_list_order);
  assert(iterator->nstr->type == NSTR_LIST_T);
  assert(iterator->nstr->refs == 1);

  // check if there will be integer overflow
  die_if((SIZE_MAX - iterator->nstr->len) < nstr->len);

  if ( iterator->i >= nstr_list_order ) {
    void **next = nstr_list_new_lst();

    die_if(next == 0);

    iterator->lst[iterator->i] = next;
    iterator->lst = next;
    iterator->i = 0;
  }

  iterator->nstr->len += nstr->len;
  iterator->lst[iterator->i] = nstr;
  ++(iterator->i);

  ret = 0;
end:
  return ret;

err:
  ret = 1;
  goto end;

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
  die_if(nstr->len >= SIZE_MAX);

  // alloc buffer for the compacted nstr
  const size_t buflen = nstr->len + 1;
  buf = malloc(buflen);
  die_if(buf == 0);

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

  die_if(buf == 0);

  char *bufi = buf;

  void **lst = nstr->lst;
  void **next;
  size_t i;
  nstr_t *subnstr;

  while(lst != 0) {
    for(i = 0; i < nstr_list_order; ++i) {
      subnstr = lst[i];

      if_unlikely(subnstr == 0) {
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
      if_unlikely(nstr_sub_compact(nstr)) {

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
      if_unlikely(nstr_list_compact(nstr)) {
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

static int
nstr_list_merge(
    nstr_list_iterator_t *itr,
    nstr_t *nstr) {

  int ret;

  assert(nstr);
  assert(nstr->type == NSTR_LIST_T);
  assert(nstr->refs == 1);

  void **lst = nstr->lst;
  void **next;
  size_t i;

  while(lst != 0) {
    for(i = 0; i < nstr_list_order; ++i) {
      if ( lst[i] == 0 ) {
        free(lst);
        goto loopexit;
      }

      die_if(nstr_list_iterator_append(itr, lst[i]));
    }

    next = lst[i];
    free(lst);
    lst = next;
  }

loopexit:
  ret = 0;

end:
  free(nstr);
  return ret;

err:
  while(lst != 0) {
    while(i < nstr_list_order) {
      nstr_unref(lst[i]);
      ++i;
    }

    next = lst[i];
    free(lst);
    lst = next;
  }
  ret = 1;
  goto end;

}

static int
nstr_buf_merge(
    nstr_t *a,
    nstr_t *b) {

  int ret;

  assert(a);
  assert(b);

  assert(a->refs == 1);
  assert(
      a->type == NSTR_BUFFER_T ||
      a->type == NSTR_BUFFER_CSTR_T);

  assert(b->len != 0);

  size_t restlen = a->buf.len - a->len;

  if ( restlen < b->len ) {
    ret = 1;
    goto end;
  }

  size_t offset = a->buf.str - a->buf.start;
  if ( (restlen - offset) < b->len ) {
    memmove(
        a->buf.start,
        a->buf.str,
        a->len);
    a->buf.str = a->buf.start;
  }

  nstr_copy_to_buf(
      &(a->buf.str[a->len]),
      b,
      0,
      b->len);

  if ( a->type == NSTR_BUFFER_CSTR_T )
    a->type = NSTR_BUFFER_T;

  a->len += b->len;

  nstr_unref(b);

  ret = 0;
end:
  return ret;

}

static nstr_t *
nstr_list_new() {

  nstr_t *ret = malloc(sizeof(*ret));
  if ( nstr_likely(ret != 0) ) {
    void **lst = nstr_list_new_lst();
    die_if(lst == 0);
    ret->lst = lst;
  }
  ret->type = NSTR_LIST_T;
  ret->len  = 0;
  ret->refs = 1;

end:
  return ret;

err:
  free(ret);
  ret = 0;
  goto end;
}

nstr_t *
nstr_concat(
    nstr_t *instr,
    ...) {

  va_list ap;
  va_start(ap, instr);

  nstr_t *ret;
  nstr_t *nnstr = 0;

  nstr_list_iterator_t itr;

  // skip empty strings at the begining
  while(1) {
    ldie_if(err_unref_args, (instr == 0));
    if ( instr->len != 0 )
      break;
    nstr_unref(instr);
    instr = va_arg(ap, nstr_t *);
  }

  // compact first element if possable
  if ( instr->refs == 1 ) {
    switch(instr->type) {
      case NSTR_BUFFER_T:
      case NSTR_BUFFER_CSTR_T:
        while(1) {
          // peek next argument
          nnstr = va_arg(ap, nstr_t *);

          // instr is now last string, so return it
          if ( nnstr == 0 ) {
            ret = instr;
            goto end;
          }

          // skip empty
          if ( nnstr->len == 0 ) {
            nstr_unref(nnstr);
            continue;
          }

          if ( nstr_buf_merge(instr, nnstr) )
            break;
        }
        break;

      case NSTR_LIST_T:
        // if first element is list
        // use it as return string
        ret = instr;
        nstr_list_iterator_init(&itr, ret);
        while(nstr_list_iterator_next(&itr) != 0);
        instr = va_arg(ap, nstr_t *);
        goto append;

      default:
        ;
    }
  }

  // create new list
  ret = nstr_list_new();
  nstr_list_iterator_init(&itr, ret);

  // take care of peeked argument
  if ( nnstr != 0 ) {
    if_unlikely(nstr_list_iterator_append(&itr, instr)) {
      nstr_unref(nnstr);
      goto err_append;
    }

   instr = nnstr;
  }

append:
  // append to the list
  while(instr !=0) {
continue_loop:
    // skip empty strings
    if_unlikely(instr->len == 0) {
      nstr_unref(instr);
      continue;
    }

    // check if we can optimize string for cheap
    if ( instr->refs == 1 ) {
      switch(instr->type) {

        case NSTR_BUFFER_T:
        case NSTR_BUFFER_CSTR_T:
          // compact buffers if possable
          while(1) {
            nnstr = va_arg(ap, nstr_t *);

            // instr was last element
            if_unlikely(nnstr == 0) {
              die_if(nstr_list_iterator_append(&itr,instr));
              goto end;
            }

            // skip empty
            if ( nnstr->len == 0 ) {
              nstr_unref(nnstr);
              continue;
            }

            if ( nstr_buf_merge(instr, nnstr) ) {
              // could not merge
              if_unlikely(
                  nstr_list_iterator_append(
                    &itr,
                    instr)) {
                nstr_unref(nnstr);
                goto err_append;
              }

              instr = nnstr;
              goto continue_loop;

            }

          }
          break;

        case NSTR_LIST_T:
          ldie_if(err_append, nstr_list_merge(&itr, instr));
          instr = va_arg(ap, nstr_t *);
          continue;

        default:
          ;
      }
    }

    ldie_if(err_append, nstr_list_iterator_append(&itr, instr));

    instr = va_arg(ap, nstr_t *);
  }

end:
  va_end(ap);
  return ret;

err_append:
  nstr_unref(ret);
  goto err_unref_args;

err_unref_args:
  // clean up args in error case
  while(instr != 0) {
    nstr_unref(instr);
    instr = va_arg(ap, nstr_t *);
  }

err:
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
