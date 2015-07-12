#include "pnc_readers.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void pnc_read_bool_from_console(char *message, bool *val) {
  pnc_read_bool_from_console_optional(message, val, false);
}

void pnc_read_bool_from_console_optional(char *message, bool *val, bool optional) {
  int attempt_count = 0;
  char input[4];

  do {
    if (attempt_count > 0) puts("Invalid input.");

    printf("%s? (Enter Yes/No or Y/N ) %s\n", message,
        optional ? " (Optional input. You can skip by pressing enter)" : "");
    printf("> ");

    fgets(input, sizeof input, stdin);
    input[strlen(input) - 1] = '\0';
    attempt_count++;
  } while ((input == NULL || strlen(input) == 0 ||
        (!equals_ic(input, "yes") && !equals_ic(input, "no") &&
         !equals_ic(input, "Y") && !equals_ic(input, "N") &&
         !equals_ic(input, "y")  && !equals_ic(input, "n"))) && !optional);

  *val  = (equals_ic(input, "Y") || equals_ic(input, "y") ||
      equals_ic(input, "yes")) ? true : false;
  printf("%s: %s\n", message, *val ? "true" : "false");
}

void pnc_read_string_from_console(char *message, char *val, int size) {
  pnc_read_string_from_console_optional(message, val, size, false);
}

void pnc_read_string_from_console_optional(char *message, char *val, int size, bool optional) {
  int attempt_count = 0;

  do {
    if (attempt_count > 0) puts("Invalid input.");

    printf("%s %s\n", message, optional ? " ( Optional input. You can skip by pressing enter )" : "");
    printf("> ");

    if(!fgets(val, size, stdin)) {
      puts("Failed to get input");
    }

    attempt_count++;
  } while ((val == NULL || strlen(val) == 1) && !optional);

  val[(int) strlen(val) - 1] = '\0';
  printf("%s : %s\n", message, val);
}

void pnc_read_int_from_console(char *message, int *val) {
  pnc_read_int_from_console_optional(message, val, false);
}

void pnc_read_int_from_console_optional(char *message, int *val, bool optional) {
  int attempt_count = 0;
  char buf[8];

  do {
    if (attempt_count > 0) puts("Invalid input.");

    printf("Enter %s %s\n", message, optional ? " ( Optional input. You can skip by pressing enter )" : "");
    printf("> ");
    fgets(buf, sizeof buf, stdin);
    *val = atoi(buf);

    attempt_count++;
  } while (*val < -1 && !optional);

  printf("%s: %d\n", message, *val);
}
