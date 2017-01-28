#include <nparse.h>
#include <nstr.h>
#include <stdlib.h>
#include <unistd.h>

#define sizeofarray(arr) \
  (sizeof(arr) / sizeof(arr[0]))

#define if_unlikely(cond) if ( nstr_unlikely(cond) )
#define if_likely(cond)   if ( nstr_likely(cond)   )

static const char *
nparser_status_str_list[] = {
  [NPARSER_OK]      = "OK, no error",
  [NPARSER_UNKNOWN] = "unknown error",
  [NPARSER_NOMEM]   = "no memory"
};

const char *
nparser_status_str(
    nparse_status_t status) {

  if_unlikely(status >= sizeofarray(nparser_status_str_list)) {
    status = NPARSER_UNKNOWN;
  }

  return nparser_status_str_list[status];

}

nparser_t *
nparser_new() {

  nparser_t *ret = malloc(sizeof(*ret));

  if_likely(ret != 0) {

    ret->cur    =  0;
    ret->end    =  0;
    ret->buffer =  0;
    ret->length =  0;
    ret->fd     = -1;

  }

  return ret;

}

nparse_status_t
nparser_open_file(
    nparser_t *parser,
    const char *fname) {

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
