#include <stdio.h>
#include <stdarg.h>

#if _MSC_VER < 1900

int snprintf(char *buffer, size_t n, const char *format, ...)
{
    va_list argp;
    int ret;
    va_start(argp, format);
    ret = vsnprintf_s(buffer, n, _TRUNCATE, format, argp);
    va_end(argp);
    return ret;
}

#endif  /* _MSC_VER < 1900 */
