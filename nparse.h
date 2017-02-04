// Copyright (c) 2017 Andrej Gelenberg <andrej.gelenberg@udo.edu>
#ifndef _NPARSE_H
#define _NPARSE_H

struct nparser_t {
  size_t      i;
  size_t      end;
  const char *buffer;
  int         fd;
  nstr_t     *nstr;
  int         is_mmap;
};
typedef struct nparser_t nparser_t;

enum nparse_status_t {
  NPARSER_OK         = 0,
  NPARSER_UNKNOWN    = 1,
  NPARSER_NOMEM      = 2,
  NPARSER_OPEN_FAIL  = 3,
  NPARSER_MMAP_FAIL  = 4,
  NPARSER_FSTAT_FAIL = 5,
  NPARSER_TODO       = 6,
  NPARSER_EMPTY      = 7,
};
typedef enum nparse_status_t nparse_status_t;

const char *
nparse_status_str(
    nparse_status_t status);

nparser_t *
nparser_new();

nparse_status_t
nparser_open_file(
    nparser_t *parser,
    const char *fname);

void
nparser_close(
    nparser_t *parser);

void
nparser_destroy(
    nparser_t *parser);

#endif // _NPARSE_H
