#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "process.h"

int main(int argc, char* argv[]) {

  char * p1 = "./11111";
  char * f1 = "test.gr";
  const char * p = get_path_to_datafile(p1, f1);
  assert(strcmp(p, "./test.gr") == 0);

  p1 = "../11111/oe";
  f1 = "test.gr";
  p = get_path_to_datafile(p1, f1);
  assert(strcmp(p, "../11111/test.gr") == 0);

  p1 = "../11111/oe/";
  f1 = "test.gr";
  p = get_path_to_datafile(p1, f1);
  assert(strcmp(p, "../11111/oe/test.gr") == 0);

  p1 = "./";
  f1 = "test.gr";
  p = get_path_to_datafile(p1, f1);
  assert(strcmp(p, "./test.gr") == 0);

  p1 = "dfd";
  f1 = "test.gr";
  p = get_path_to_datafile(p1, f1);
  assert(strcmp(p, "test.gr") == 0);

  char pa[PATH_MAX] = {0};
  f1 = "t";
  for(size_t i = 0; i < PATH_MAX-2; i++){
    pa[i] = '/';
  }
  p = get_path_to_datafile(pa, f1);
  assert(strlen(p) == PATH_MAX-1);

  f1 = "tt";
  p = get_path_to_datafile(pa, f1);
  assert(p == NULL);

  FILE * in;
  State_Processing st;

  in = fopen("test_data/no_pair0.txt", "r");
  clear_read_state();
  assert(in != NULL);
  st = found_next_header(in);
  fclose(in);
  assert(st == END_OPERATION);

  in = fopen("test_data/no_pair1.txt", "r");
  clear_read_state();
  assert(in != NULL);
  st = found_next_header(in);
  fclose(in);
  assert(st == END_OPERATION);

  in = fopen("test_data/no_pair2.txt", "r");
  clear_read_state();
  st = found_next_header(in);
  fclose(in);
  assert(st == END_OPERATION);

  in = fopen("test_data/no_pair3.txt", "r");
  clear_read_state();
  st = found_next_header(in);
  fclose(in);
  assert(st == END_OPERATION);

  in = fopen("test_data/only_head_mess.txt", "r");
  clear_read_state();
  st = found_next_header(in);
  fclose(in);
  assert(st == PARSE_MESSAGE);

  in = fopen("test_data/only_head_mask.txt", "r");
  clear_read_state();
  st = found_next_header(in);
  fclose(in);
  printf("%d\n", st);
  assert(st == PARSE_MASK);

  in = fopen("test_data/header_chain.txt", "r");
  clear_read_state();
  st = found_next_header(in);
  assert(st == PARSE_MASK);
  st = found_next_header(in);
  assert(st == PARSE_MASK);
  st = found_next_header(in);
  assert(st == PARSE_MESSAGE);
  st = found_next_header(in);
  assert(st == PARSE_MESSAGE);
  st = found_next_header(in);
  assert(st == PARSE_MASK);
  st = found_next_header(in);
  assert(st == PARSE_MESSAGE);
  st = found_next_header(in);
  assert(st == PARSE_MASK);
  fclose(in);
  printf("%d\n", st);
  assert(st == PARSE_MASK);
  

  in = fopen("test_data/no_pair1.txt", "r");
  clear_read_state();
  uint32_t mask = 0;
  st = parse_mask(in, &mask);
  fclose(in);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/no_pair2.txt", "r");
  clear_read_state();
  mask = 0;
  st = parse_mask(in, &mask);
  fclose(in);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/mask_even_error.txt", "r");
  clear_read_state();
  mask = 0;
  st = parse_mask(in, &mask);
  fclose(in);
  assert(st == END_OPERATION);

  in = fopen("test_data/mask1.txt", "r");
  clear_read_state();
  mask = 0;
  st = parse_mask(in, &mask);
  fclose(in);
  assert(st == OK_OPERATION);
  assert(mask == 0x12345678);

  struct Message_Struct message;
  in = fopen("test_data/simple_message.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == OK_OPERATION);
  assert(message.type == 0xFF);
  assert(message.length == 1);
  assert(message.paylaod.data.data8[0] == 0xA5);
  assert(message.paylaod.crc32 == 0xa8e282d1);
  
  in = fopen("test_data/err_crc_s_message.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == OK_OPERATION);
  assert(message.valid_crc == false);

  in = fopen("test_data/err_len_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == OK_OPERATION);
  assert(message.valid_crc == false);

  in = fopen("test_data/err_len1_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == OK_OPERATION);
  assert(message.valid_crc == false);

  in = fopen("test_data/err_len2_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == END_OPERATION);
  assert(message.valid_crc == false);

  in = fopen("test_data/err_no_crc_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == END_OPERATION);
  assert(message.valid_crc == false);

  in = fopen("test_data/err_no_data_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  printf("st %d\n", st);
  assert(st == OK_OPERATION);
  assert(message.valid_crc == true);

  in = fopen("test_data/err_corrupt_data_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == ERROR_OPERATION);
  assert(message.valid_crc == true);

  in = fopen("test_data/err_big_len_mess.txt", "r");
  clear_read_state();
  st = parse_message(in, &message);
  fclose(in);
  assert(st == ERROR_OPERATION);
  assert(message.valid_crc == true);

  message.length = 0;
  mask = 0;
  pad_and_mask(mask, &message);
  assert(message.length == 0);
  assert(message.paylaod.crc32 == 0xffffffff);

  message.length = 1;
  mask = 0;
  pad_and_mask(mask, &message);
  assert(message.length == 4);
  assert(message.paylaod.crc32 != 0xffffffff);

  message.length = 252;
  mask = 0;
  pad_and_mask(mask, &message);
  assert(message.length == 252);

  message.length = 11;
  for(int i =0; i < 11/4; i++)
    message.paylaod.data.octets[i] = 0xFFFFFFFF;
  mask = 0;
  pad_and_mask(mask, &message);
  assert(message.length == 12);
  assert(message.paylaod.data.octets[0] == 0);
  assert(message.paylaod.data.octets[2] == 0);

  message.type = 0xAA;

  printf("=====\n");

  FILE * out = fopen("test_data/mess.txt", "w");
  File_State ft;
  ft = write_message(out, &message, false);
  assert(ft == FILE_OK);
  ft = write_message(out, &message, true);
  assert(ft == FILE_OK);
  ft = write_all(out);
  assert(ft == FILE_OK);
  fclose(out);

  message.length = 9;
  message.paylaod.crc32 = 0x12345678;
  for(int i =0; i < 9/4; i++)
    message.paylaod.data.octets[i] = 0xFFFFFFFF;
  mask = 0;
  out = fopen("test_data/process_1_mess.txt", "w");
  st = conver_and_write_message(out, mask, &message);
  ft = write_all(out);
  assert(ft == FILE_OK);
  fclose(out);
  assert(st == OK_OPERATION);
  
  message.length = 9;
  message.paylaod.crc32 = 0x12345678;
  for(int i =0; i < 9/4; i++)
    message.paylaod.data.octets[i] = 0xFFFFFFFF;
  mask = 0;
  out = fopen("test_data/process_2_mess.txt", "w");
  st = conver_and_write_message(out, mask, &message);
  assert(st == OK_OPERATION);  

  message.length = 15;
  message.paylaod.crc32 = 0x12345678;
  message.valid_crc = false;
  for(int i =0; i < 19/4; i++)
    message.paylaod.data.octets[i] = 0xFFFFFFFF;
  mask = 0;
  st = conver_and_write_message(out, mask, &message);
  assert(st == OK_OPERATION);
  ft = write_all(out);
  assert(ft == FILE_OK);
  fclose(out);

  in = fopen("test_data/process_chain_mess_in.txt", "r");
  out = fopen("test_data/process_chain_mess_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == END_OPERATION);

  in = fopen("test_data/process_double_mask_in.txt", "r");
  out = fopen("test_data/process_double_mask_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  printf("st %d\n", st);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_double_mess_in.txt", "r");
  out = fopen("test_data/process_double_mess_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_only_1_mess_in.txt", "r");
  out = fopen("test_data/process_only_1_mess_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_error_mask_in.txt", "r");
  out = fopen("test_data/process_error_mask_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_error_short_mask_in.txt", "r");
  out = fopen("test_data/process_error_short_mask_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_error_short_mess_in.txt", "r");
  out = fopen("test_data/process_error_short_mess_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);

  in = fopen("test_data/process_error_long_mess_in.txt", "r");
  out = fopen("test_data/process_error_long_mess_out.txt", "w");
  st = process_files(in , out);
  fclose(in);
  fclose(out);
  assert(st == ERROR_OPERATION);
  
  return 0;
}
