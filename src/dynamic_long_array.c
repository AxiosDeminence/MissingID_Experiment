#include <stdlib.h>
#include <stdio.h>
#include "dynamic_long_array.h"

int append(long element, struct dynamic_long_array *dynamic_array) {
  if (at_capacity(dynamic_array)) {
    int result = extend_array(dynamic_array);
    if (result != 0) {
      return result;
    }
  }

  dynamic_array->array[dynamic_array->len++] = element;
  printf("len: %zu | capacity: %zu\n", dynamic_array->len,
      dynamic_array->capacity);
  return 0;
}

int at_capacity(struct dynamic_long_array *dynamic_array) {
  return dynamic_array->len == dynamic_array->capacity;
}

int extend_array(struct dynamic_long_array *dynamic_array) {
  if (at_capacity(dynamic_array) &&
      dynamic_array->capacity * 2 < dynamic_array->capacity) {
    /* If the array is not at capacity or the size would overflow*/
    fprintf(stderr, "Increasing capacity past %zu will exceed system's"
                    "implementation of size_t causing an overflow\n",
                    dynamic_array->capacity);
    return -1;
  }
  dynamic_array->capacity *= 2;
  long *new_ptr = realloc(dynamic_array->array,
                          sizeof(long) * dynamic_array->capacity);
  if (new_ptr == NULL) {
    /* If there were some issue with expanding the dynamic array */
    fprintf(stderr, "Error reallocating dynamic long array with new capacity"
                    "%zu\n", dynamic_array->capacity);
    return 1;
  }
  dynamic_array->array = new_ptr;
  return 0;
}

struct dynamic_long_array create_long_dynamic_array(size_t initial_capacity,
                                                    int *err_no) {
  long *underlying_array = calloc(initial_capacity, sizeof(long));
  struct dynamic_long_array dynamic_array = {
    underlying_array, 0, initial_capacity
  };
  if (underlying_array == NULL) {
    fprintf(stderr, "Failed creating dynamic array with initial capacity of"
                    "%zu\n", initial_capacity);    
    *err_no = 1;
  } else {
    *err_no = 0;
  }
  return dynamic_array; 
}
