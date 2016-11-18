#include <stdio.h>
#include <nstr.h>

#undef NDEBUG
#include <assert.h>

int
main(
    int  argc,
    char **argv) {

  int ret = 0;
  const char *str;

  nstr_t *s1 = nstr_new_cstr("str1");
  assert(s1);
  assert(s1->type == NSTR_CSTR_T);
  assert(s1->len == 4);
  assert(strcmp(s1->cstr.str, "str1") == 0);

  nstr_t *s2 = nstr_new_const("str2", 4);
  assert(s2);
  assert(s2->type == NSTR_CONST_T);
  assert(s2->len == 4);

  char *s3str = strdup("str3");
  assert(s3str);
  nstr_t *s3 =
    nstr_new_buffer(
      s3str, s3str, 4, 5);
  assert(s3);
  assert(s3->type == NSTR_BUFFER_CSTR_T);
  assert(s3->len == 4);
  assert(strcmp(s3->buf.str, "str3") == 0);

  char *s4str = strdup("str4X");
  assert(s4str);
  nstr_t *s4 =
    nstr_new_buffer(
      s4str, s4str, 4, 5);
  assert(s4);
  assert(s4->type == NSTR_BUFFER_CSTR_T);
  assert(s4->len == 4);
  assert(s4->buf.len == 5);
  assert(strcmp(s4->buf.str, "str4") == 0);

  char *s5str = strdup("Xstr5");
  assert(s5str);
  nstr_t *s5 =
    nstr_new_buffer(
       s5str, &(s5str[1]), 4, 6);
  assert(s5);
  assert(s5->type == NSTR_BUFFER_CSTR_T);
  assert(s5->len == 4);
  assert(s5->buf.len == 6);
  assert(strcmp(s5->buf.str, "str5") == 0);

  char *s6str = strdup("Xstr6X");
  assert(s6str);
  nstr_t *s6 =
    nstr_new_buffer(
      s6str, &(s6str[1]), 4, 7);
  assert(s6);
  assert(s6->type == NSTR_BUFFER_CSTR_T);
  assert(s6->len == 4);
  assert(s6->buf.len == 7);
  assert(strcmp(s6->buf.str, "str6") == 0);

  char *s7str = malloc(4);
  assert(s7str);
  memcpy(s7str, "str7", 4);
  nstr_t *s7 =
    nstr_new_buffer(
      s7str, s7str, 4, 4);
  assert(s7);
  assert(s7->type == NSTR_BUFFER_T);
  assert(s7->len == 4);
  assert(s7->buf.len == 4);

  char *s8str = malloc(5);
  assert(s8str);
  memcpy(s8str, "Xstr8", 5);
  nstr_t *s8 =
    nstr_new_buffer(
      s8str, &(s8str[1]), 4, 5);
  assert(s8);
  assert(s8->type == NSTR_BUFFER_T);
  assert(s8->len == 4);
  assert(s8->buf.len == 5);

  nstr_t *s9ref = nstr_new_cstr("Xstr9");
  assert(s9ref);

  nstr_t *s9 =
      nstr_new_sub(
          s9ref,
          1,
          4);
  assert(s9);
  assert(s9->type == NSTR_SUB_T);
  assert(s9->len == 4);
  assert(s9->sub.ref == s9ref);
  assert(s9->sub.offset == 1);
  assert(s9ref->refs == 2);

  nstr_unref(s9ref);
  assert(s9ref->refs == 1);

  nstr_t *s10ref =
      nstr_new_cstr(
          "Xstr10X");
  assert(s10ref);

  nstr_t *s10 =
      nstr_new_sub(
          s10ref,
          1,
          5);

  nstr_unref(s10ref);

#define ck_str(n) \
    str = nstr_cstr(s ## n); \
    assert(str); \
    assert(strcmp(str, "str" # n) == 0);

  ck_str(1);
  ck_str(2);
  ck_str(3);
  ck_str(4);
  ck_str(5);
  ck_str(6);
  ck_str(7);
  assert(s7->type == NSTR_BUFFER_CSTR_T);
  assert(s7->buf.len == 5);
  assert(strcmp(s7->buf.str, "str7") == 0);

  ck_str(8);
  assert(s8->type == NSTR_BUFFER_CSTR_T);
  assert(strcmp(s8->buf.str, "str8") == 0);

  ck_str(9);
  assert(s9->type == NSTR_SUB_T);

  ck_str(10);
  assert(s10->type == NSTR_BUFFER_T);
  assert(s10->len == 5);
  assert(s10->buf.str == s10->buf.start);
  assert(s10->buf.len == s10->len + 1);

  nstr_unref(s1);
  nstr_unref(s2);
  nstr_unref(s3);
  nstr_unref(s4);
  nstr_unref(s5);
  nstr_unref(s6);
  nstr_unref(s7);
  nstr_unref(s8);
  nstr_unref(s9);
  nstr_unref(s10);

  return ret;

}
