/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pb_strncasecmp.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define NUMBER_OF_STRINGS 2

int pb_strncasecmp(const char *str1, const char *str2, size_t number_of_chars)
{
	if (str1 == NULL || str2 == NULL) {
		return -1;
	}

	const char* strings[NUMBER_OF_STRINGS] = {str1, str2};
	char copied_strings[NUMBER_OF_STRINGS][number_of_chars];

	for (int s = 0; s < NUMBER_OF_STRINGS; s++) {
		memset(copied_strings[s], '\0', number_of_chars);
		strncpy(copied_strings[s], strings[s], number_of_chars);

		for (int c = 0; c < number_of_chars; c++) {
			copied_strings[s][c] = tolower(copied_strings[s][c]);
		}
	}

	return strncmp(copied_strings[0], copied_strings[1], number_of_chars);	
}
