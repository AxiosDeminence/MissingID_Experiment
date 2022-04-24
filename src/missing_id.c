#include <stdio.h>
#include <stdlib.h>
#include "missing_id.h"
#include "dynamic_long_array.h"
#include "csv.h"

long missing_number(long *array, size_t len) {
  /**
   * Define n = array_len and [-1, k] to be the range of elements.
   * We allow [-1, 0] to be in the range for different ways to track missing IDs
   * but are not considered valid IDs for the missing number
   * Solution requirements:
   * - Must be performed without changing the already allocated array
   * - Preferably, without sorting. Sorting would provide an easier
   *   implementation but will result in a minimum O(nlog(n)) time. 
   * - k (known >200k) will be much larger than n (likely <1000) in its initial
   *   iterations, meaning radix sort would not be a suitable option.
   * Solution:
   * - Push all non-positive values to the left of the array and keep track of
   *   index of the first positive value (unused for the original intended
   *   problem)
   * - Take the subarray of only positive values as arr_{pos} and the length of
   *   the subarray is n_{pos}.
   * - Iterate through arr_{pos} such that if 1 <= abs(elem) <= n_{pos}, then
   *   arr_{pos}[abs(elem)-1] will go from positive to negative. The absolute
   *   value is used since we know all elements in the subarray were intended
   *   to be positive but might be made into negatives.
   * - Iterate through arr_{pos} again such that if elem > 0, then that
   *   idx(elem) must not have been found in arr_{pos}. Since we subtract by 1
   *   earlier to constrain the range of indices edited from [0, n_{pos}], we
   *   should add by one again in order to understand what abs(elem) was not
   *   found in the array.
   * - If all values in the positive array was made negative, then it should be
   *   safe to assume that all positive numbers [1, n_{pos}] were found such
   *   that the smallest missing positive number was n_{pos} + 1;
   * - We can then reiterate through arr_{pos} and then set them back to
   *   positive. This takes O(1) space, O(n) time.
   **/
  
  size_t i = 0;
  size_t first_pos = 0;
  size_t minimum_missing_positive;
  /* Push all of the non-positive numbers to the left */
  for (i = 0; i < len; ++i) {
    if (array[i] < 1) {
      long tmp = array[i];
      array[i] = array[first_pos];
      /**
       * We increment the first_pos after making the last_non_pos or first_pos-1
       * into a negative.
       **/
      array[first_pos++] = tmp;
    }
 }

  /* unsigned int pos_len = len - first_pos; */
  for (i = first_pos; i < len; ++i) {
    /* Elem should always be at least 1. */
    long elem = (array[i] > 0) ? array[i] : -array[i];
    /* An interesting way to keep the shift active */

    /* Keep the index within the index range */
    if (elem + first_pos - 1 < len && array[elem + first_pos - 1] > 0) {
      array[elem + first_pos - 1] *= -1;
    }
  }

  minimum_missing_positive = len - first_pos + 1;
  for (i = first_pos; i < len; ++i) {
    /**
     * If the elmeent at index i is still positive, it was not found in the
     * array
     **/
    if (array[i] > 0 && minimum_missing_positive == len - first_pos + 1) {
      minimum_missing_positive = i - first_pos + 1;
    }
    /* Reset the array to the previous state */
    if (array[i] < 0) {
      array[i] *= -1;
    }
  }
  return minimum_missing_positive;
} 

/**
 * We use the strtol function here which requires a null terminating character.
 * Therefore, do NOT use it without specifying the CSV_APPEND_NULL option
 **/
void field_callback(void *s, size_t len, void *data) {
  struct parser_info *info = (struct parser_info *)data;
  int retval;
  if ((info->ignore_headers && !info->past_header) ||
       info->current_column++ != info->id_column) {
    return;
  }

  /**
   * Technically if ignore_headers is not set, then it will still work since
   * strtol will return 0 when it reaches the header as long as the header does
   * not start with a numeric value\
   **/
  retval = append(strtol((char *)s, NULL, 10), info->array);

  if (retval != 0) {
    fprintf(stderr, "Some error occurred while reading a field: %d", retval);
    exit(EXIT_FAILURE);
  }
}

void record_callback(int c, void *data) {
  /**
   * c here is either the line terminator unsigned char or -1 if the final
   * record was never line terminated, either way it isn't used
   **/
  struct parser_info *info = (struct parser_info *)data;
  info->past_header = 1;
  info->current_column = 0;
}  

struct dynamic_long_array compile_ids_from_files(const char* const* filenames,
    const long *columns, size_t len, int ignore_headers, unsigned char quote,
    unsigned char token, size_t starting_capacity, int *err_no) {  
  struct dynamic_long_array dynamic_array;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read, i;

  *err_no = 0;
  dynamic_array = create_long_dynamic_array(starting_capacity, err_no);
  if (*err_no != 0) {
    return dynamic_array;
  }

  if (csv_init(&p, CSV_STRICT & CSV_APPEND_NULL & CSV_EMPTY_IS_NULL) != 0) {
    fprintf(stderr, "Error creating csv parser\n");
    *err_no = 1;
  }
  csv_set_delim(&p, token);
  csv_set_quote(&p, quote);

  for (i = 0; i < len; ++i) {
    struct parser_info parser_info;
    FILE *file;

    parser_info.array = &dynamic_array;
    parser_info.ignore_headers = ignore_headers;
    parser_info.id_column = columns[i];
    parser_info.past_header = 0;
    parser_info.current_column = 0;
    /* filenames should be null terminated */
    file = fopen(filenames[i], "r");
    if (file == NULL) {
      fprintf(stderr, "Error opening file: %s\n", filenames[i]);
      *err_no = 2;
      fclose(file);
      csv_free(&p);
      return dynamic_array;
    }
    while ((bytes_read=fread(buf, 1, 1024, file)) > 0) {
      if (csv_parse(&p, buf, bytes_read, field_callback, record_callback,
            &parser_info) != bytes_read) {
        fprintf(stderr, "Error while parsing file: %s\n",
            csv_strerror(csv_error(&p)));
        *err_no = 3;
        fclose(file);
        csv_free(&p);
        return dynamic_array;
      }
    }
    fclose(file);
  }

  csv_free(&p);
  return dynamic_array;
}
