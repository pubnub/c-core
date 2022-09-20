/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PB_DEPRECATED
# define INC_PB_DEPRECATED

# if defined(_MSC_VER)
#  if _MSC_VER >= 1310
#   define PUBNUB_DEPRECATED __declspec(deprecated)
#   define PUBNUB_DISABLE_WARNING_PUSH __pragma(warning(push))
#   define PUBNUB_DISABLE_WARNING_POP __pragma(warning(pop))
#   define PUBNUB_DISABLE_DEPRACATED __pragma(warning(disable: 4101))
#  endif

# elif defined(__GNUC__)
#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
#   define PUBNUB_DEPRECATED __attribute__((deprecated))
#   define PUBNUB_DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
#   define PUBNUB_DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")
#   define PUBNUB_DISABLE_DEPRECATED _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
# endif

# elif
#  define PUBNUB_DEPRECATED 
#  define PUBNUB_DISABLE_WARNING_PUSH
#  define PUBNUB_DISABLE_WARNING_POP
#  define PUBNUB_DISABLE_DEPRACATED
# endif

#endif
