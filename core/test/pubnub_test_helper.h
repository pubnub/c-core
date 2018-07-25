#if PUBNUB_RECEIVE_GZIP_RESPONSE        
/* 'Accept-Encoding' header line */
#define ACCEPT_ENCODING "Accept-Encoding: gzip\r\n"
#else
#define ACCEPT_ENCODING ""
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */
