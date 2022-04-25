#include <stddef.h>
#include <string.h>

#include <node_api.h>
#include "dynamic_long_array.h"
#include "missing_id.h"

#define NAPI_CALL(env, call, cb)                                      \
  do {                                                                \
    napi_status status = (call);                                      \
    if (status != napi_ok) {                                          \
      const napi_extended_error_info* error_info = NULL;              \
      napi_get_last_error_info((env), &error_info);                   \
      const char* err_message = error_info->error_message;            \
      bool is_pending;                                                \
      napi_is_exception_pending((env), &is_pending);                  \
      if (!is_pending) {                                              \
        const char* message = (err_message == NULL)                   \
            ? "empty error message"                                   \
            : err_message;                                            \
        napi_throw_error((env), NULL, message);                       \
        (cb);                                                         \
      }                                                               \
    }                                                                 \
  } while(0)

void util_free_filename_array(char **array, size_t len) {
  size_t i;
  for (i = 0; i < len; ++i) {
    free(array[i]);
  }
  free(array);
}

static void free_arraybuffer(napi_env env, void *data, void *hint) {
  free(data);
}

static napi_value napi_missing_number(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_value result;

  /* Used for processing the first argument */
  bool is_typedarray;
  napi_typedarray_type underlying_type;
  size_t length;
  long* array;

  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL), NULL);

  if (argc != 1) {
    NAPI_CALL(env, napi_throw_error(env, "ERR_MISSING_ARGS", "Incorrect number of args provided."), NULL);
    return NULL;
  }

  NAPI_CALL(env, napi_is_typedarray(env, argv[0], &is_typedarray), NULL);
  if (!is_typedarray) {
    NAPI_CALL(env, napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "Does not pass in a TypedArray."), NULL);
    return NULL;
  }

  NAPI_CALL(env, napi_get_typedarray_info(env, argv[0], &underlying_type, &length, (void **)&array, NULL, NULL), NULL);
  if (underlying_type != napi_bigint64_array) {
    NAPI_CALL(env, napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "TypedArray is not of BigInt64."), NULL);
    return NULL;
  }

  NAPI_CALL(env, napi_create_bigint_uint64(env, missing_number(array, length), &result), NULL);
  return result;
}

static napi_value napi_compile_ids(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  /* Used to check if an argument is a correct type */
  bool correct_type;
  /* Used for building arguments for native function */
  uint32_t num_of_files;
  /* long columns[num_of_files]; */
  /* char *files[num_of_files]; */

  /* Used quote and delimiter string fields */
  size_t field_len;
  char quote;
  char delimiter;

  /* Used for native add-on call and returning function */
  int err_no;
  napi_value long_arraybuffer;
  napi_value result;
  napi_finalize finalizer;

  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL), NULL);

  if (argc != 3) {
    NAPI_CALL(env, napi_throw_error(env, "ERR_MISSING_ARGS", "Incorrect number of args provided."), NULL);
    return NULL;
  }

  /* Begin processing second argument: Quote character */

  /* Will throw an error since there is no napi_string check */
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &field_len), NULL);
  if (field_len != 1) {
    /* If the argument is not a single character */
    NAPI_CALL(env, napi_throw_error(env, "ERR_INVALID_ARG_VALUE",
        "Quote field must be exactly one character"), NULL);
  }

  char str[2];
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], str,
      2 * sizeof(char), NULL), NULL);
  quote = str[0];

  /* Begin processing second argument: Delimiter character */

  /* Will throw an error since there is no napi_string check */
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], NULL, 0, &field_len), NULL);
  if (field_len != 1) {
    /* If the argument is not a single character */
    NAPI_CALL(env, napi_throw_error(env, "ERR_INVALID_ARG_VALUE",
        "Quote field must be exactly one character"), NULL);
    return NULL;
  }

  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], str,
      2 * sizeof(char), NULL), NULL);
  delimiter = str[0];


  /* Begin processing first argument: array of filenames */

  NAPI_CALL(env, napi_is_array(env, argv[0], &correct_type), NULL);
  if (!correct_type) {
    NAPI_CALL(env, napi_throw_type_error(env, "ERR_INVALID_ARG_TYPE", "Does not pass in a TypedArray for argument 0."), NULL);
  }
  /* Must use a normal Array since there is no typedarray for strings */
  NAPI_CALL(env, napi_get_array_length(env, argv[0], &num_of_files), NULL);

  char **files = calloc(num_of_files, sizeof(char *));
  long columns[num_of_files];

  /**
   * The highest possible index is 2^32 - 2 since the max array length
   * is 2^32 - 1. Thus preventing overflows as we will always be less
   * than MAX_UINT32.
   **/
  for (uint32_t i = 0; i < num_of_files; ++i) {
    napi_handle_scope scope;
    napi_value element;
    size_t strlen;
    char *str;
    columns[i] = 0;

    NAPI_CALL(env, napi_open_handle_scope(env, &scope), util_free_filename_array(
        files, num_of_files));

    NAPI_CALL(env, napi_get_element(env, argv[0], i, &element), util_free_filename_array(
        files, num_of_files));

    NAPI_CALL(env, napi_get_value_string_utf8(env, element, NULL, 0, &strlen),
        util_free_filename_array(files, num_of_files));

    str = calloc(strlen + 1, sizeof(char));
    if (str == NULL) {
      NAPI_CALL(env, napi_throw_error(env, "ERR_MEMORY_ALLOCATION_FAILED",
          "Failed to memory for string buffer on heap"), util_free_filename_array(
              files, num_of_files));
    }

    /* Always appends a null terminator expected for relevant functions */
    NAPI_CALL(env, napi_get_value_string_utf8(env, element,
        str, (strlen+1) * sizeof(char), NULL), util_free_filename_array(
            files, num_of_files));
    
    files[i] = str;
    NAPI_CALL(env, napi_close_handle_scope(env, scope), util_free_filename_array(
        files, num_of_files));
  }

  struct dynamic_long_array dynamic_array = 
      compile_ids_from_files((const char * const *)files, columns, num_of_files, 0, (unsigned char)quote,
          (unsigned char)delimiter, 0, &err_no);
  
  util_free_filename_array(files, num_of_files);
  
  if (err_no != 0) {
    NAPI_CALL(env, napi_throw_error(env, "ERR_OPERATION_FAILED",
        "Failed to initialize the csv parser"), free_dynamic_long_array(&dynamic_array));
  }
  
  /**
   * I think these calls result in a pointer being directed towards somewhere
   * in the middle of the arraybuffer resulting in valgrind reporting a 
   * it being possibly lost. It might be that we look at the byte_offset 0
   * and the length and other attributes of the arraybuffer are accessed
   * relative to some position in the arraybuffer.
   **/
  NAPI_CALL(env, napi_create_external_arraybuffer(env, (void *)dynamic_array.array, 
      dynamic_array.len * sizeof(long), free_arraybuffer, NULL,
      &long_arraybuffer), NULL);

  NAPI_CALL(env, napi_create_typedarray(env, napi_bigint64_array, dynamic_array.len,
      long_arraybuffer, 0, &result), NULL);

  return result;
}

NAPI_MODULE_INIT() {
  napi_property_descriptor bindings[] = {
    {"missingID", NULL, napi_missing_number, NULL, NULL, NULL, napi_default_method, NULL},
    {"compileIDs", NULL, napi_compile_ids, NULL, NULL, NULL, napi_default_method, NULL},
  };

  NAPI_CALL(env, napi_define_properties(env, exports, sizeof(bindings) / sizeof(napi_property_descriptor), bindings), NULL);

  return exports;
}