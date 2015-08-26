#include "pnc_helpers.h"

void pnc_read_bool_from_console(char const *message, bool *val);
void pnc_read_bool_from_console_optional(char const *message, bool *val, bool optional);

void pnc_read_string_from_console(char const *message, char *val, int size);
void pnc_read_string_from_console_optional(char const *message, char *val, int size, bool optional);

void pnc_read_int_from_console(char const *message, int *input);
void pnc_read_int_from_console_optional(char const *message, int *input, bool optional);
