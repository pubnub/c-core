/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/// This defines the `PUBNUB_EXTERN` macro, which is used to declare
/// functions that are part of the Pubnub C client library API.
/// Users still can override this, if they want to, by defining
/// `PUBNUB_EXTERN` before including any Pubnub header.

#ifndef PUBNUB_EXTERN
#ifdef __cplusplus
#define PUBNUB_EXTERN extern "C"
#else
#define PUBNUB_EXTERN extern
#endif
#endif /* PUBNUB_EXTERN */

