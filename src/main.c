#include <argp.h>
#include <string.h>
#include <stdlib.h>

#include "dynamic_long_array.h"
#include "missing_id.h"

/**
 * Prints the gaps between IDs of a given formatted file. Assumes that each
 * line, has the same number of features and is separated by the same delimiter.
 * Additionally assumes that the ID is the same feature and can be represented
 * as some integer.
 *
 * By default, negative IDs are disregarded as some erroneous input.
 *
 * Valgrind identified a still reachable block allocated by the GNU argp
 * library when we perform argp_usage or argp_error. Apparently this isn't
 * really that much of a worry since the OS should be reclaiming all of the
 * memory when the process ends (which it will do immediately once executing
 * those functions), and it will always be a constant size. This is due to the
 * fact that it is based off of the group size (all kept default), number of 
 * child inputs, and number of options. If all options are parsed normally, then
 * we do not end the process early allowing the argp_parse function to
 * eventually free the aforementioned block. In essence, it works.
 *
 * Requires GNU99 for <argp.h>
 **/

const char *argp_program_version = "v0.1";
const char *argp_program_bug_address = "juhmertena@gmail.com";

static const char doc[] = "A program used to find missing IDs in the similarly"
                          "formatted files, tokenized by a delimiter.";

static const char arg_docs[] = "missing-id-lister [options] [FILES...]";

/*https://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html*/
static struct argp_option options[] = {
  { "quote", 't', "QUOTE", 0, "Quote character (default \") for the files"},
  { "token", 't', "DELIMITER", 0, "Delimiter (default commas) in the files" },
  { "headers", 'h', 0, 0, "Ignore headers in files (default false)" },
  { "output", 'o', "OUTPUT_FILE", 0, "File to output to" },
  { "input", 'i', "INPUT_FILE(s)", 0, "File(s) to search through" },
  { "columns", 'c', "ID COLUMN #", 0,
    "Column that has the ID (default first column)" },
  { 0 }
};

struct arguments { 
  /**
   * We could technically make different quote and token characters for each
   * input file like the columns, but I think that's more trouble than what it's
   * worth. It isn't impossible, but I'd like to imagine that the user has some
   * control over the format of the extracted data. In my intended use case, the
   * user will have control over the quote and delimiter characters for every
   * file, but might not have ease of control over the ID column.
   **/
  unsigned char quote;
  unsigned char token;
  int ignore_headers;
  char *output;

  size_t input_file_length;
  size_t column_specify_length;
  char **input;
  long *columns;
};

void FreeArguments(struct arguments *arguments) {
  free(arguments->input);
  free(arguments->output);
  free(arguments->columns);
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case ARGP_KEY_INIT:
      arguments->quote = '"';
      arguments->token = ',';
      arguments->ignore_headers = 0;
      arguments->output = NULL;

      arguments->input = NULL;
      arguments->columns = NULL;
      arguments->input_file_length = 0;
      arguments->column_specify_length = 0;
      break;
    case 'q': case 't':
    {
      char special_char;
      /**
       * Only accept one character strings (libcsv limitation). Binary data
       * allowed. Prevent use newline/carriage returns (needed to delimit rows).
       * Also prevent empty input. Due to limitations of argp, we will not check
       * for redeclarations.
       **/
      if (strlen(arg) != 1) {
        FreeArguments(arguments);
        argp_error(state, "Quote/Delimiter may not be empty");
      }
      special_char = arg[0];

      switch (special_char) {
        case '\n':
        case '\r':
          FreeArguments(arguments);
          argp_error(state, "Quote/Delimiter may not be CR or LF");
          break;
        default:
          /* Any allowed characters may be used */
          if (key == 'q') {
            arguments->quote = special_char;
          } else { /* (key == 't') */
            arguments->token = special_char;
          }
          break;
      }
      break;
    }
    case 'h':
      arguments->ignore_headers = 1;
      break;
    case 'o':
      /* Prevent empty input. Otherwise, any file name would be valid */
      arguments->output = arg;
      break;
    case 'i':
    case ARGP_KEY_ARG:
    {
      /**
       * Try to open the file specified in the argument. If it does not exist
       * then it is considered as an error.
       **/
      FILE *file;
      if ((file = fopen(arg, "r")) == NULL) {
        FreeArguments(arguments);
        argp_error(state, "File %s does not exist", arg);
      }
      fclose(file);

      /**
       * The argument is on the stack, allowing us to just save the reference
       * to the argument.
       **/
      arguments->input = realloc(arguments->input,
          (arguments->input_file_length + 1) * sizeof(char *));
      arguments->input[arguments->input_file_length++] = arg;
      break;
    }
    case 'c':
    {
      /**
       * Since arg is a pointer to the beginning of a string, then we can just
       * use that and tokenize it.
       **/
      char *checker = strtok(arg, " ");
      while (checker != NULL) {
        long num = strtol(checker, &checker, 10);
        if (*checker != '\0' || num < 0) {
          /* If there was non-decimal characters*/
          FreeArguments(arguments);
          argp_error(state, "Non-numeric columns not allowed");
        }
        arguments->columns = realloc(arguments->columns,
            (arguments->column_specify_length + 1) * sizeof(long));
        arguments->columns[arguments->column_specify_length++] = num;
        checker = strtok(NULL, " ");
      }
      break;
    }
    case ARGP_KEY_END:
      if (arguments->input == NULL) {
        FreeArguments(arguments);
        argp_error(state, "Input file not given");
      } else if (arguments->quote == arguments->token) {
        FreeArguments(arguments);
        argp_error(state, "Quote and token cannot be same character");
      }

      /**
       * If the columns were never specified, then set it here It's a lot
       * easier to handle this for the default instead of at init so we don't
       * need to write code to overwrite only the first instance.
       **/
      if (arguments->column_specify_length == 0) {
        arguments->column_specify_length = 1;
        arguments->columns = calloc(1, sizeof(long));
        if (arguments->columns == NULL) {
          FreeArguments(arguments);
          argp_error(state, "Memory Error");
        }
        arguments->columns[0] = 0;
      }
      if (arguments->column_specify_length == 1) {
        long *new_columns;
        unsigned int i;
        /* If only one column is specified, make sure it's used for all files */
        arguments->column_specify_length = arguments->input_file_length;
        new_columns = realloc(arguments->columns,
            sizeof(long) * arguments->column_specify_length);
        if (new_columns == NULL) {
          FreeArguments(arguments);
          argp_error(state, "Memory error");
        }
        arguments->columns = new_columns;

        for (i = 1; i < arguments->input_file_length; ++i) {
          arguments->columns[i] = arguments->columns[0];
        }
      } else if (arguments->column_specify_length != 
          arguments->input_file_length) {
        /**
         * If number of specified columns is not equal to the number of input
         * files, then print error
         **/
        FreeArguments(arguments);
        argp_error(state, "Number of columns do not match number of input files");
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
      break;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, arg_docs, doc };

int main(int argc, char *argv[]) {
  struct arguments arguments;
  struct dynamic_long_array dynamic_array;
  int ret_val = 0;

  argp_parse( &argp, argc, argv, 0, 0, &arguments );

  dynamic_array = compile_ids_from_files((const char* const *)arguments.input,
      arguments.columns, arguments.input_file_length, arguments.ignore_headers,
      arguments.quote, arguments.token, 100, &ret_val);
  if (ret_val != 0) {
    FreeArguments(&arguments);
    free_long_dynamic_array(&dynamic_array);
    exit(EXIT_FAILURE);
  }
  printf("Missing id: %ld\n", missing_number(dynamic_array.array,
        dynamic_array.len));

  FreeArguments(&arguments);

  free_long_dynamic_array(&dynamic_array);

  exit(EXIT_SUCCESS);
}
