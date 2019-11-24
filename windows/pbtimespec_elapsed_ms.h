/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined(INC_PBMC_ELAPSED_MS)
#define INC_PBMC_ELAPSED_MS

#include <windows.h>

int pbtimespec_elapsed_ms(FILETIME prev_timspec, FILETIME timspec);

#endif /* !defined(INC_PBMC_ELAPSED_MS) */
