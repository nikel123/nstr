#include <nstr.h>

void
nstr_copy_to_buf(
    char   *target,
    nstr_t *nstr,
    size_t offset,
    size_t len) {

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

    case NSTR_EMPTY_T:
      break;

  }

}

static nstr_t *
nstr_sub_compact(
    nstr_t *nstr) {

  char   *buf;
  nstr_t *ret;

  assert(nstr != 0);
  assert(nstr->type == NSTR_SUB_T);
  assert(nstr->sub.ref->type != NSTR_EMPTY_T);

  // check for int overflow
  if ( nstr_unlikely(nstr->len >= SIZE_MAX) )
    goto err;

  // alloc buffer for the compacted nstr
  const size_t buflen = nstr->len + 1;
  buf = malloc(nstr->len + 1);
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

  ret = nstr;

end:
  return ret;

err:
  ret = 0;
  goto end;

}

/* campact the nstr
 (returns NSTR_BUFFER_CSTR_T)

 if returns not 0, then don't use nstr argument anymore
 can be already freed
 if return 0, then nstr should stay unmodified
*/
nstr_t *
nstr_compact(
    nstr_t *nstr) {

  nstr_t *ret;

  switch(nstr->type) {
    case NSTR_EMPTY_T:
    case NSTR_CONST_T:
    case NSTR_CSTR_T:
    case NSTR_BUFFER_T:
    case NSTR_BUFFER_CSTR_T:
      ret = nstr;
      break;

    case NSTR_SUB_T:
      ret = nstr_sub_compact(nstr);
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
      nstr = nstr_sub_compact(nstr);

      if ( nstr_likely(nstr != 0) ) {
        assert(nstr->type == NSTR_BUFFER_CSTR_T);
        ret = nstr->cstr.str;
      } else {
        ret = 0;
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

    case NSTR_EMPTY_T:
      ret = "";
      break;

  }

  return ret;

}
