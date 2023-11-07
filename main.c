#include <stdbool.h>
#include "process.h"

#if !(defined(__x86_64__) && defined(__linux__))
  #error Build only X64 and Linux Platform
#endif

#define _OUT_FILE "data_out.txt"
#define _IN_FILE "data_in.txt"

const char *in_file = _IN_FILE;
const char *out_file = _OUT_FILE;

const char *get_path_to_datafile(const char *path, const char *file_name);

int main(int argc, char *argv[]) {

  (void)argc;

  FILE *out, *error_file;
  FILE *in;
  int ret_code = 0;

  const char *exe_path = argv[0];
  const char *full_path_to_out_file = get_path_to_datafile(exe_path, out_file);

  // to do: check file type
  out = fopen(full_path_to_out_file, "w");
  if (out == NULL) {
    perror("Can't open " _OUT_FILE " file");
    fprintf(stderr, "relative path to " _OUT_FILE " : %s",
            full_path_to_out_file);
    return -1;
  }

  error_file = out;

  const char *full_path_to_in_file = get_path_to_datafile(exe_path, in_file);
  // to do: check file type
  in = fopen(full_path_to_in_file, "r");
  if (in == NULL) {
    perror("Can't open " _IN_FILE " file");
    fprintf(stderr, "relative path to " _IN_FILE " : %s\n",
            full_path_to_in_file);
    fprintf(out, "Can't open " _IN_FILE " file. Relative path: %s\n",
            full_path_to_in_file);
    ret_code = -1;
    goto exit;
  }

  ret_code = process_files(in, out) == END_OPERATION ? 0 : -1;

exit:
  fclose(out);
  if (in != NULL)
    fclose(in);
  return ret_code;
}
