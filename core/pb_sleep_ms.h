/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if defined _WIN32
void pb_sleep_ms(DWORD dwMilliseconds);
#else
void pb_sleep_ms(long milliseconds);
#endif
