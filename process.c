
#include <string.h>

#include "common.h"
#include "process.h"

void pad_and_mask(const uint32_t mask, struct Message_Struct *message) {
  const uint32_t pad_len = (message->length + 3) / 4 * 4;
  message->length = pad_len;

  const uint32_t octet_len = message->length / 4;

  for (uint32_t i = 0; i < octet_len; i += 2) {
    message->paylaod.data.octets[i] &= mask;
  }

  const uint32_t calced_crc32 =
      calc_crc32(message->paylaod.data.data8, message->length);

  message->paylaod.crc32 = calced_crc32;
}

#define _BUFFER_SIZE 512 * 2
static char buffer[_BUFFER_SIZE + 1] = {0};
static size_t inner_position = 0;

File_State write_all(FILE *out) {
  if (inner_position == 0)
    return FILE_OK;
  const size_t write_len = fwrite(buffer, 1, inner_position, out);
  if (write_len != inner_position)
    return FILE_ERROR;

  inner_position = 0;
  return FILE_OK;
}

static File_State check_and_writer(FILE *out) {
  if (inner_position >= _BUFFER_SIZE)
    return write_all(out);

  return FILE_OK;
}

static File_State write_symbol(FILE *out, const char s) {
  buffer[inner_position] = s;
  inner_position += 1;
  return check_and_writer(out);
}

static File_State write_hex(FILE *out, uint8_t b) {
  if (sprintf(buffer + inner_position, "%.2x", b) < 0)
    return FILE_ERROR;
  inner_position += 2;
  return check_and_writer(out);
}

#define WRITE_BYTE_AND_CHECK(out, b)                                           \
  ({                                                                           \
    st = write_hex(out, b);                                                    \
    if (st != FILE_OK)                                                         \
      return FILE_ERROR;                                                       \
  })

File_State write_message(FILE *out, const struct Message_Struct *message,
                         const bool skip_type) {

  File_State st;

  if (skip_type == false)
    WRITE_BYTE_AND_CHECK(out, message->type);

  WRITE_BYTE_AND_CHECK(out, message->length);

  for (size_t i = 0; i < message->length; i++)
    WRITE_BYTE_AND_CHECK(out, message->paylaod.data.data8[i]);

  uint8_t *p = (uint8_t *)&message->paylaod.crc32;
  for (size_t i = 0; i < sizeof(message->paylaod.crc32); i++)
    WRITE_BYTE_AND_CHECK(out, p[i]);

  return FILE_OK;
}

#define CHECK_FILE_OP(st)                                                      \
  ({                                                                           \
    if (st != FILE_OK) {                                                       \
      return ERROR_OPERATION;                                                  \
    }                                                                          \
  })

#define FLUSH_AND_CHECK(out)                                                   \
  ({                                                                           \
    if (write_all(out) != FILE_OK)                                             \
      return ERROR_OPERATION;                                                  \
  })

static File_State error_log(FILE *out, const char *fmt, ...) {
  if (write_all(out) != FILE_OK)
    return FILE_ERROR;
  va_list args;
  va_start(args, fmt);
  int r = vfprintf(out, fmt, args);
  va_end(args);
  if (r < 0) {
    perror("Can't write error message to log file");
    return FILE_ERROR;
  }
  return FILE_OK;
}

#define ERROR_LOG_CHECK(out, ...)                                              \
  ({                                                                           \
    if (error_log(out, __VA_ARGS__) != FILE_OK)                                \
      return ERROR_OPERATION;                                                  \
  })

State_Processing conver_and_write_message(FILE *out, const uint32_t mask,
                                          struct Message_Struct *message) {

  if (message->valid_crc == false) {
    ERROR_LOG_CHECK(out, "Error: invalid CRC:");
  }

  File_State st = write_message(out, message, false);
  CHECK_FILE_OP(st);

  pad_and_mask(mask, message);

  st = write_message(out, message, true);
  CHECK_FILE_OP(st);

  st = write_symbol(out, '\n');
  CHECK_FILE_OP(st);

  return OK_OPERATION;
}

State_Processing process_files(FILE *in, FILE *out) {

  State_Processing state = FIND_NEXT_ANY_HEADER;
  State_Processing new_state = FIND_NEXT_ANY_HEADER;

  struct Mask_Struct mask_st;
  mask_st.ready = false;
  struct Message_Struct message;
  message.ready = false;

  clear_read_state();

  while (true) {

    switch (state) {
    case FIND_MASK_HEADER:
    case FIND_MESSAGE_HEADER:
    case FIND_NEXT_ANY_HEADER:
      new_state = found_next_header(in);
      if (state == FIND_MASK_HEADER)
        if (new_state != PARSE_MASK) {
          ERROR_LOG_CHECK(out, "Unable to locate a corresponding mask for the "
                               "parsed message.\n");
          state = ERROR_OPERATION;
          break;
        }
      if (state == FIND_MESSAGE_HEADER)
        if (new_state != PARSE_MESSAGE) {
          ERROR_LOG_CHECK(out, "Error: Unable to locate a corresponding "
                               "message for the parsed mask.\n");
          state = ERROR_OPERATION;
          break;
        }

      state = new_state;
      break;

    case PARSE_MASK:
      new_state = parse_mask(in, &mask_st.mask);
      if (new_state != OK_OPERATION) {
        ERROR_LOG_CHECK(out, "Error: Failed to parse the mask.\n");
        state = ERROR_OPERATION;
        break;
      }
      mask_st.ready = true;
      state = PROCESSING;
      break;

    case PARSE_MESSAGE:
      new_state = parse_message(in, &message);
      if (new_state != OK_OPERATION) {
        ERROR_LOG_CHECK(out, "Error: Failed to parse the message.\n");
        state = ERROR_OPERATION;
        break;
      }
      message.ready = true;
      state = PROCESSING;
      break;

    case PROCESSING:

      if (mask_st.ready && message.ready) {
        mask_st.ready = false;
        message.ready = false;
        new_state = conver_and_write_message(out, mask_st.mask, &message);
        memset(&mask_st, 0, sizeof(mask_st));
        memset(&message, 0, sizeof(message));
        if (new_state != OK_OPERATION) {
          state = ERROR_OPERATION;
        } else
          state = FIND_NEXT_ANY_HEADER;
      } else if (mask_st.ready) {
        state = FIND_MESSAGE_HEADER;
      } else if (message.ready) {
        state = FIND_MASK_HEADER;
      }
      break;

    case ERROR_OPERATION:
      ERROR_LOG_CHECK(out, "There was an error processing.\n");
      const size_t pos = get_total_position_in_file();
      ERROR_LOG_CHECK(out, "Position in input file %zu\n", pos);
      return ERROR_OPERATION;

    case END_OPERATION:
      FLUSH_AND_CHECK(out);
      return END_OPERATION;

    default:
      fprintf(stderr, "undefined state");
      return ERROR_OPERATION;
    }
  }
}
