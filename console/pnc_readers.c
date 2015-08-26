/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pnc_readers.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


void pnc_read_bool_from_console(char const *message, bool *val)
{
    pnc_read_bool_from_console_optional(message, val, false);
}


static bool valid_bool_input(char const *s)
{
    return (s != NULL) &&
        (equals_ic(s, "yes") || equals_ic(s, "no") ||
         equals_ic(s, "Y") || equals_ic(s, "N"));
}


void pnc_read_bool_from_console_optional(char const *message, bool *val, bool optional)
{
    char input[4];
    bool first_try = true;
    
    do {
        if (!first_try) {
            puts("Invalid input. Try again.");
        }
        
        printf("%s? (Enter Yes/No or Y/N ) %s\n> ", message,
               optional ? " (Optional input. You can skip by pressing enter)" : "");
        
        fgets(input, sizeof input, stdin);
        chomp(input);
        first_try = false;
    } while (!valid_bool_input(input) && !optional);

    *val  = (equals_ic(input, "Y") || equals_ic(input, "yes"));
    printf("%s: %s\n", message, *val ? "true" : "false");
}


void pnc_read_string_from_console(char const *message, char *val, int size)
{
    pnc_read_string_from_console_optional(message, val, size, false);
}


void pnc_read_string_from_console_optional(char const *message, char *val, int size, bool optional)
{
    bool first_try = true;
    
    do {
        if (!first_try) {
            puts("Invalid input. Try again.");
        }
        
        printf("%s %s\n> ", message, optional ? " ( Optional input. You can skip by pressing enter )" : "");
        
        if (!fgets(val, size, stdin)) {
            puts("Failed to get input");
        }
        
        first_try = false;
    } while ((val == NULL || strlen(val) == 1) && !optional);
    
    chomp(val);
    printf("%s : %s\n", message, val);
}


void pnc_read_int_from_console(char const *message, int *val)
{
    pnc_read_int_from_console_optional(message, val, false);
}


void pnc_read_int_from_console_optional(char const *message, int *val, bool optional) {
    char buf[8];
    bool first_try = true;
    
    do {
        if (!first_try) {
            puts("Invalid input. Try again.");
        }
        
        printf("Enter %s %s\n", message, optional ? " ( Optional input. You can skip by pressing enter )" : "");
        printf("> ");
        fgets(buf, sizeof buf, stdin);
        *val = atoi(buf);
        
        first_try = false;
    } while (*val < -1 && !optional);
    
    printf("%s: %d\n", message, *val);
}
