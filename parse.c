#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "process.h"

#define _BUFFER_SIZE 8
static char buffer[_BUFFER_SIZE] = {0};
static size_t read_bytes = 0;
static size_t total_position_in_file = 0;
static size_t inner_position = 0;
static bool exit_file = false;

size_t get_total_position_in_file() { return total_position_in_file; }

void clear_read_state(void) {
  read_bytes = 0;
  total_position_in_file = 0;
  inner_position = 0;
  exit_file = false;
}

// no thread safety
static File_State read_byte_from_file(FILE *in, char *s) {

  bool read_new_chunk = inner_position >= read_bytes;

  if (read_new_chunk && exit_file)
    return READ_END;

  total_position_in_file++;

  read_new_chunk |= read_bytes == 0;
  read_new_chunk &= !exit_file;

  if (read_new_chunk) {
    read_bytes = fread(buffer, 1, _BUFFER_SIZE, in);
    if (read_bytes != _BUFFER_SIZE) {
      if (ferror(in) != 0) {
        return FILE_ERROR;
      }
      exit_file = true;
    }

    inner_position = 0;
  }

  *s = buffer[inner_position];
  inner_position++;

  return FILE_OK;
}

#define READ_AND_CHECK_ST(s, st)                                               \
  ({                                                                           \
    st = read_byte_from_file(in, &s);                                          \
    if (st == FILE_ERROR)                                                      \
      return ERROR_OPERATION;                                                  \
    if (st == READ_END)                                                        \
      return END_OPERATION;                                                    \
  })

State_Processing found_next_header(FILE *in) {

  char s;
  File_State st;
  union {
    char buf[sizeof(uint64_t)];
    uint64_t reg64;
  } head;

  head.reg64 = 0;
  const size_t in_position = strlen("mask=") - 1;

  while (true) {

    READ_AND_CHECK_ST(s, st);
    head.buf[in_position] = s;

    if (head.reg64 == *(uint64_t *)"mask=") {
      return PARSE_MASK;
    }
    if (head.reg64 == *(uint64_t *)"mess=") {
      return PARSE_MESSAGE;
    }

    head.reg64 >>= 8;
  }

  return ERROR_OPERATION;
}

static State_Processing hex2uint32(const char *str, uint32_t *val) {
  char *endptr;
  *val = (uint32_t)strtol(str, &endptr, 16);

  if (errno != 0 && val == 0) {
    perror("errpr strtol");
    return ERROR_OPERATION;
  }

  if (endptr == str) {
    fprintf(stderr, "No digits were found\n");
    return ERROR_OPERATION;
  }

  if (*endptr != '\0') { /* Not necessarily an error... */
    fprintf(stderr, "No hex number in the mask: %s\n", endptr);
    return ERROR_OPERATION;
  }

  return OK_OPERATION;
}

#define _HEX_32BIT_SIZE (sizeof(uint32_t) * 2)

State_Processing parse_mask(FILE *in, uint32_t *mask) {

  char str[_HEX_32BIT_SIZE + 1] = {0};
  File_State st;

  for (size_t i = 0; i < _HEX_32BIT_SIZE; i++) {
    READ_AND_CHECK_ST(str[i], st);
  }

  return hex2uint32(str, mask);

  return OK_OPERATION;
}

#define HEX2UINT32_CHECK(str, val)                                             \
  ({                                                                           \
    if (hex2uint32(str, &val) != OK_OPERATION)                                 \
      return ERROR_OPERATION;                                                  \
  })

State_Processing parse_message(FILE *in, struct Message_Struct *message) {

  char str[_HEX_32BIT_SIZE + 1] = {0};
  File_State st;

  READ_AND_CHECK_ST(str[0], st);
  READ_AND_CHECK_ST(str[1], st);
  uint32_t type;
  HEX2UINT32_CHECK(str, type);
  message->type = type & 0xFF;

  READ_AND_CHECK_ST(str[0], st);
  READ_AND_CHECK_ST(str[1], st);
  uint32_t len;
  HEX2UINT32_CHECK(str, len);

  if (MAX_LENGTH_DATA < len) {
    fprintf(stderr, "Message length is too long\n");
    return ERROR_OPERATION;
  }

  message->length = len;

  memset(message->paylaod.data.data8, 0, MAX_LENGTH_DATA);

  uint32_t byte;
  for (uint32_t i = 0; i < len; i++) {
    READ_AND_CHECK_ST(str[0], st);
    READ_AND_CHECK_ST(str[1], st);
    HEX2UINT32_CHECK(str, byte);
    message->paylaod.data.data8[i] = byte & 0xFF;
  }

  for (size_t i = 0; i < _HEX_32BIT_SIZE; i++) {
    READ_AND_CHECK_ST(str[i], st);
  }

  uint32_t crc32;
  HEX2UINT32_CHECK(str, crc32);

  const uint32_t calced_crc32 = calc_crc32(message->paylaod.data.data8, len);

  message->valid_crc = calced_crc32 == crc32;

  message->paylaod.crc32 = crc32;

  return OK_OPERATION;
}
