// Copyright (c) 2017 Andrej Gelenberg <andrej.gelenberg@udo.edu>
#ifndef _NPARSE_H
#define _NPARSE_H

struct nparser_t {
  nstr_iterator_t *itr;
  nstr_t          *buffer;
  size_t           length;
  int              fd;
};
typedef struct nparser_t nparser_t;

enum nparse_status_t {
  NPARSER_OK      = 0,
  NPARSER_UNKNOWN = 1,
  NPARSER_NOMEM   = 2,
};
typedef enum nparse_status_t nparse_status_t;

const char *
nparser_status_str(
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
