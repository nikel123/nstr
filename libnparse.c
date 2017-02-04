#include <nparse.h>
#include <nstr.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define sizeofarray(arr) \
  (sizeof(arr) / sizeof(arr[0]))

#define if_unlikely(cond) if ( nstr_unlikely(cond) )
#define if_likely(cond)   if ( nstr_likely(cond)   )

static const char *
nparse_status_str_list[] = {
  [NPARSER_OK]         = "OK, no error",
  [NPARSER_UNKNOWN]    = "unknown error",
  [NPARSER_NOMEM]      = "no memory",
  [NPARSER_OPEN_FAIL]  = "can't open file",
  [NPARSER_MMAP_FAIL]  = "mmap() failed",
  [NPARSER_FSTAT_FAIL] = "fstat() failed",
  [NPARSER_TODO]       = "some function is not implemented",
};

const char *
nparse_status_str(
    nparse_status_t status) {

  if_unlikely(status >= sizeofarray(nparse_status_str_list)) {
    status = NPARSER_UNKNOWN;
  }

  return nparse_status_str_list[status];

}

nparser_t *
nparser_new() {

  nparser_t *ret = malloc(sizeof(*ret));

  if_likely(ret != 0) {

    ret->i       =  0;
    ret->end     =  0;
    ret->buffer  =  0;
    ret->fd      = -1;
    ret->nstr    =  0;
    ret->is_mmap =  0;

  }

  return ret;

}

static nparse_status_t
nparser_mmap_open(
    nparser_t *parser) {

  int c;
  nparse_status_t ret;
  struct stat st;

  c = fstat(parser->fd, &st);

  if_unlikely(c == -1)
    goto err_fstat;

  if_unlikely(st.st_size == 0)
    goto err_empty;

  parser->buffer = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, parser->fd, 0);

  if_unlikely(parser->buffer == MAP_FAILED)
    goto err_mmap;

  parser->end     = st.st_size;
  parser->is_mmap = 1;

  ret = NPARSER_OK;
end:
  return ret;

err_fstat:
  ret = NPARSER_FSTAT_FAIL;
  goto end;

err_mmap:
  ret = NPARSER_MMAP_FAIL;
  goto end;

err_empty:
  ret = NPARSER_EMPTY;
  goto end;

}

static nparse_status_t
nparser_stream_open(
    nparser_t *parser) {

  return NPARSER_TODO;

}

nparse_status_t
nparser_open_file(
    nparser_t *parser,
    const char *fname) {

  nparse_status_t ret;
  const char *buffer;

  parser->fd = open(fname, O_RDONLY);

  if_unlikely(parser->fd == -1)
    goto err;

  ret = nparser_mmap_open(parser);

  if_unlikely( ret != NPARSER_OK ) {
    fprintf(
        stderr,
        "nparser_mmap_open() failed %s: %m\n",
        nparse_status_str(ret));
    ret = nparser_stream_open(parser);
  }

end:
  return ret;

err:
  ret = NPARSER_OPEN_FAIL;
  goto end;

}

void
nparser_close(
    nparser_t *parser) {

  if ( parser->fd != -1 ) {
    close(parser->fd);
  }

}

nparser_destroy(
    nparser_t *parser) {

  nparser_close(parser);
  free(parser);

}
