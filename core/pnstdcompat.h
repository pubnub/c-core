#pragma once

#ifdef __APPLE__
#	define sprintf_s snprintf
#elif defined(__linux) || defined(__linux__)
#	define sprintf_s snprintf
#else
#endif