#ifndef DYNAMIC_LONG_ARRAY_H
#define DYNAMIC_LONG_ARRAY_H

struct dynamic_long_array {
  long *array;
  size_t len;
  size_t capacity;
};

int append(long element, struct dynamic_long_array *dynamic_array);

int at_capacity(struct dynamic_long_array *dynamic_array);

int extend_array(struct dynamic_long_array *dynamic_array);

struct dynamic_long_array create_long_dynamic_array(size_t initial_capacity,
                                                    int *err_no);

#endif
