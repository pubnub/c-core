#include "pnc_helpers.h"

void pnc_read_bool_from_console(char *message, bool *val);
void pnc_read_bool_from_console_optional(char *message, bool *val, bool optional);

void pnc_read_string_from_console(char *message, char *val, int size);
void pnc_read_string_from_console_optional(char *message, char *val, int size, bool optional);

void pnc_read_int_from_console(char *message, int *input);
void pnc_read_int_from_console_optional(char *message, int *input, bool optional);
