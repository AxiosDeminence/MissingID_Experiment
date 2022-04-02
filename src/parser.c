#include "csv.h"

static long curr_column = 0;

struct parser_info {
   long *array;
   size_t len;
   size_t capacity;
   long column;    
}

void cb1 (void *s, size_t len, void *data) {
  /* We use exits here since the required signature of these callback functions
   * may only return voids. */
  struct parser_info info = *data;
  if (curr_column != info.column) {
    return;
  }

  info.array[len] = strtol

  if (info.len == info.capacity) {
    if (info.capacity * 2 < info.capacity) {
      /* If it would result in an overflow */
      exit(EXIT_FAILURE);
    }
    int *new_ptr = realloc(array, info.capacity * 2);
    if (new_ptr == NULL) {
      /* Reallocation has failed */
      exit(EXIT_FAILURE);
    } else {
      array = new_ptr;
      info.capacity *= 2;
    }
  }
}
