#include <stdint.h>

const char *get_path_to_datafile(const char *path, const char *file_name);

uint32_t calc_crc32(const uint8_t *buf, int len);
