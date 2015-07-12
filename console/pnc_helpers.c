#include "pnc_helpers.h"
#include <string.h>

bool equals_ic(char *s1, char *s2) {
  return strcasecmp(s1, s2) == 0;
}
