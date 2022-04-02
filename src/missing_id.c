#include <stdio.h>
#include <stdlib.h>
#include "missing_id.h"
#include "dynamic_long_array.h"

long missing_number(long *array, size_t len) {
  /* Define n = array_len and [-1, k] to be the range of elements.
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
   */
  
  size_t i = 0;
  size_t first_pos = 0;
  /* Push all of the non-positive numbers to the left */
  for (i = 0; i < len; ++i) {
    if (array[i] < 1) {
      long tmp = array[i];
      array[i] = array[first_pos];
      /* We increment the first_pos after making the last_non_pos or first_pos-1
       * into a negative. */
      array[first_pos++] = tmp;
    }
  }

  /* unsigned int pos_len = len - first_pos; */
  for (i = first_pos; i < len; ++i) {
    /* Elem should always be at least 1. */
    long elem = (array[i] > 0) ? array[i] : -array[i];
    /* An interesting way to keep the shift active*/
    /* Keep the index within the index range */
    if (elem + first_pos - 1 < len && array[elem + first_pos - 1] > 0) {
      array[elem + first_pos - 1] *= -1;
    }
  }

  long minimum_missing_positive = len - first_pos + 1;
  for (i = first_pos; i < len; ++i) {
    /* If the elmeent at index i is still positive, it was not found in the
     * array */
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

int add_ids_from_file(const char *filename, struct parser_info info) {
  /* Assume array_len = n and the length of ids in filename = m. We must
   * traverse through the entirety of filename such that O(m). */
  FILE* file;
  if ((file = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return kFileNotExist;
  }

  return kOk;
}

int compile_ids_from_files(const char* const* filenames, const long *columns,
    size_t len, int ignore_headers, unsigned char quote, unsigned char token,
    size_t starting_capacity) {  
  
  size_t i;
  int err_no = 0;
  struct dynamic_long_array dynamic_array = create_long_dynamic_array(
      starting_capacity, &err_no);
  if (err_no != 0) {
    return err_no;
  }

  for (i = 0; i < len; ++i) {
    struct parser_info parser_info = {dynamic_array, ignore_headers, columns[i],
                                      0, 0};
    int result = add_ids_from_file(filenames[i], parser_info);
    if (result != kOk) {
      return result;
    }
  }
  return kOk;
}


int main(int argc, char *argv[]) {
  char **filenames = &argv[1];
  long *columns = calloc(2, sizeof(long));
  if (columns == NULL) {
    return EXIT_FAILURE;
  }
  size_t len = 2;
  /* Casting and signature function prevents the pointer to the begining of a
   * string being overwritten or characters themselves being overwitten. */
  int ret_val = compile_ids_from_files((const char* const*)filenames, columns,
                                       len, 0, '"', ',', 50);
  if (ret_val != kOk) {
   return ret_val;
  } 
  return EXIT_SUCCESS;
}
