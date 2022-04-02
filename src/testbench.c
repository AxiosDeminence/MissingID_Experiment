#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "csv.h"

struct counts {
  long unsigned fields;
  long unsigned rows;
};

void cb1(void *s, size_t len, void *data) {
  ((struct counts *)data)->fields++;

  /*fprintf(stderr, "%s\n", (char *)s);*/
}

void cb2(int c, void *data) {
  ((struct counts *)data)->rows++;
  /*fprintf(stdout, "%d\n", c);*/
}

int main(int argc, char *argv[]) {
  FILE *fp;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  struct counts c = {0, 0};

  printf("%s\t%s\t%s\t%s\t%s\t\"%s\"\t\"%s\"\t\"%s\"\t%s\n", "id", "bd", "cd",
         "ld", "md", "name", "url", "purpose", "time");

  if (csv_init(&p, CSV_EMPTY_IS_NULL) != 0) exit(EXIT_FAILURE);
  fp = fopen(argv[1], "r");
  if (!fp) exit(EXIT_FAILURE);

  csv_set_delim(&p, '\t');

  while ((bytes_read=fread(buf, 1, 1024, fp)) >0) {
    if (csv_parse(&p, buf, bytes_read, cb1, cb2, &c) != bytes_read) {
      fprintf(stderr, "Error while parsing file: %s\n",
              csv_strerror(csv_error(&p)));
      exit(EXIT_FAILURE);
    }
  }

  csv_fini(&p, cb1, cb2, &c);
  fclose(fp);
  printf("%lu fields, %lu rows\n", c.fields, c.rows);

  csv_free(&p);
  exit(EXIT_SUCCESS);
}
