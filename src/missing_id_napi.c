#include <stddef.h>
#include <stdio.h>
#include <node_api.h>
#include <assert.h>
#include "missing_id.h"

#define NAPI_CALL(env, call)                                      \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      const napi_extended_error_info* error_info = NULL;          \
      napi_get_last_error_info((env), &error_info);               \
      const char* err_message = error_info->error_message;        \
      bool is_pending;                                            \
      napi_is_exception_pending((env), &is_pending);              \
      if (!is_pending) {                                          \
        const char* message = (err_message == NULL)               \
            ? "empty error message"                               \
            : err_message;                                        \
        napi_throw_error((env), NULL, message);                   \
        return NULL;                                              \
      }                                                           \
    }                                                             \
  } while(0)

static napi_value napi_missing_number(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

  if (argc != 1) {
    NAPI_CALL(env, napi_throw_error(env, "IncorrectArgc", "Incorrect number of args provided."));
    return NULL;
  }

  bool is_typedarray;
  NAPI_CALL(env, napi_is_typedarray(env, argv[0], &is_typedarray));
  if (!is_typedarray) {
    NAPI_CALL(env, napi_throw_type_error(env, "No_TypedArray", "Does not pass in a TypedArray."));
    return NULL;
  }

  napi_typedarray_type underlying_type;
  size_t length;
  long* array;
  NAPI_CALL(env, napi_get_typedarray_info(env, argv[0], &underlying_type, &length, (void **)&array, NULL, NULL));
  if (underlying_type != napi_bigint64_array) {
    NAPI_CALL(env, napi_throw_type_error(env, "Not_BigInt64_TypedArray", "TypedArray is not of BigInt64."));
    return NULL;
  }

  napi_value result;
  NAPI_CALL(env, napi_create_bigint_uint64(env, missing_number(array, length), &result));
  return result;
}

static napi_value napi_compile_ids(napi_env env, napi_callback_info info) {

}

NAPI_MODULE_INIT() {
  napi_value napi_missing_id_func;
  napi_status status;

  NAPI_CALL(env, napi_create_function(env, "findLowestMissingID", NAPI_AUTO_LENGTH,
                                      napi_missing_number, NULL, &napi_missing_id_func));

  status = napi_set_named_property(env, exports, "missingID", napi_missing_id_func);
  if (status != napi_ok) return NULL;

  return exports;
}