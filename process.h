#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum { FIND_NEXT_ANY_HEADER, FIND_MASK_HEADER, FIND_MESSAGE_HEADER, PARSE_MESSAGE, PARSE_MASK, PROCESSING, ERROR_OPERATION, END_OPERATION, OK_OPERATION } State_Processing;


typedef enum { FILE_OK, READ_END, FILE_ERROR} File_State;

#define MAX_LENGTH_DATA 252

struct Message_Struct {
  uint8_t type;
  uint8_t length;
  struct {
    union {
      uint8_t data8[MAX_LENGTH_DATA];
      uint32_t octets[MAX_LENGTH_DATA/4];
    } data;
    uint32_t crc32;
  } paylaod ;
  bool ready;
  bool valid_crc;
};

struct Mask_Struct {
  uint32_t mask;
  bool ready;
};


void clear_read_state(void);
State_Processing found_next_header(FILE * in);
State_Processing parse_mask(FILE *in, uint32_t *mask);
State_Processing parse_message(FILE *in, struct Message_Struct *message);


File_State write_message(FILE *out, const struct Message_Struct *message,  const bool skip_type );
File_State write_all(FILE *out);
size_t get_total_position_in_file();


void pad_and_mask(const uint32_t mask, struct Message_Struct *message);
State_Processing conver_and_write_message(FILE* out, const uint32_t mask, struct Message_Struct *message);
State_Processing process_files(FILE* in, FILE* out);
