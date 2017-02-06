/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBMD5_TO_STR
#define      INC_PBMD5_TO_STR


/** Stores a hex (with small, non-caps, letters) representation of an
    MD5 in @p md5_store into a statically allocated string @p out.

    @pre sizeof out >= 33
 */
#define pbmd5_to_str(md5_store, out) PUBNUB_ASSERT(sizeof out >= 33); snprintf(out, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5_store[0], md5_store[1], md5_store[2], md5_store[3], md5_store[4], md5_store[5], md5_store[6], md5_store[7], md5_store[8], md5_store[9], md5_store[10], md5_store[11], md5_store[12], md5_store[13], md5_store[14], md5_store[15]);


#endif /* !defined INC_PBMD5_TO_STR */
