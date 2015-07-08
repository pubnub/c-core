/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_VERSION
#define	INC_PUBNUB_VERSION


/** @file pubnub_version.h 
    This is the name / version API of the Pubnub client library.
    It has the functions that are present in all variants and
    platforms and have the same interface in all of them.
*/


/** Returns a string with the name of the Pubnub SDK client you
    are using.
*/
char const *pubnub_sdk_name(void);

/** Returns a string with the version of the Pubnub SDK client you are
    using.
*/
char const *pubnub_version(void);

/** Returns an URL encoded string with the full identification of the
    SDK - name, version, possible something more.
*/
char const *pubnub_uname(void);


#endif /* !defined INC_PUBNUB_VERSION */
