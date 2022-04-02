#ifndef MISSING_ID_H
#define MISSING_ID_H

#include "dynamic_long_array.h"

struct parser_info {
  struct dynamic_long_array array;
  int ignore_headers;
  long id_column;

  int past_header;
  long current_column;
};

enum Err {
  kOk,
  kFileNotExist,
  kMemoryError,
  kUnknownError
};

long missing_number(long *array, size_t len);

int add_ids_from_file(const char *filename, struct parser_info info);

int compile_ids_from_files(const char * const* filenames, const long *columns,
    size_t len, int ignore_headers, unsigned char quote, unsigned char token,
    size_t starting_capacity);

#endif
