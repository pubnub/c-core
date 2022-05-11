#pragma once

#ifdef _MSC_VER
#define sprintf(d,b,...) sprintf_s(d,sizeof(b),b,__VA_ARGS__)
#define strncpy(d,s,c) strncpy_s(d,(strlen(d)+1),s,c)
#define strcpy(d,s) strcpy_s(d,(strlen(d)+1),s)
#define strcat(d,s) strcat_s(d,strlen(d)+1,s)
#define sscanf(...) sscanf_s(__VA_ARGS__)
#define strerror(e) strerror_s(__VA_ARGS__)
#endif
