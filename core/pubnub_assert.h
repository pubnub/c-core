/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_ASSERT
#define      INC_PUBNUB_ASSERT

#include <stdbool.h>


/** The Pubnub ASSERT macros. There are several layers:
    - highest (PUBNUB_ASSERT_LEVEL_EX): all checks enabled, 
    even the long lasting ones
    - regular (PUBNUB_ASSERT_LEVEL): only the long lasting
    checks are disabled
    - lowest (PUBNUB_ASSERT_LEVEL_OPT): only the checks with
    negligible execution length are enabled
    - none (PUBNUB_ASSERT_LEVEL_NONE): all checks disabled

    Define at most one of above mentioned macros before including this
    header.  If none is defined, the highest (_EX) level is assumed.
 */

/* Let's make sure only one of @c PUBNUB_ASSERT_LEVEL_EX, @c
 * PUBNUB_ASSERT_LEVEL, @c PUBNUB_ASSERT_LEVEL_OPT or @c
 * PUBNUB_ASSERT_LEVEL_NONE is defined.
 */

#if defined(PUBNUB_ASSERT_LEVEL_EX) && (                                \
    defined(PUBNUB_ASSERT_LEVEL) || defined(PUBNUB_ASSERT_LEVEL_OPT) || \
    defined(PUBNUB_ASSERT_LEVEL_NONE) ) 
#error Cannott define PUBNUB_ASSERT_LEVEL_EX and any lower level (regular, _OPT, _NONE)
#endif

#if defined(PUBNUB_ASSERT_LEVEL) && (                                   \
    defined(PUBNUB_ASSERT_LEVEL_OPT) || defined(PUBNUB_ASSERT_LEVEL_NONE) ) 
#error Cannott define PUBNUB_ASSERT_LEVEL and any lower level (_OPT, _NONE)
#endif


#if defined(PUBNUB_ASSERT_LEVEL_OPT) && defined(PUBNUB_ASSERT_LEVEL_NONE) 
#error Cannott define PUBNUB_ASSERT_LEVEL_OPT and PUBNUB_ASSERT_LEVEL_NONE
#endif

/* If none of ASSERT level defining macros is defined, let's assume
 * the highest level.
 */

#if !defined(PUBNUB_ASSERT_LEVEL_EX) &&				\
    !defined(PUBNUB_ASSERT_LEVEL) &&				\
    !defined(PUBNUB_ASSERT_LEVEL_OPT) &&                        \
    !defined(PUBNUB_ASSERT_LEVEL_NONE)
#define PUBNUB_ASSERT_LEVEL_EX
#endif

#ifdef _MSC_VER
#define PUBNUB_ANALYSIS_ASSUME(X) __analysis_assume(X)
#else
#define PUBNUB_ANALYSIS_ASSUME(X)
#endif

/** The common ASSERT implementation */
#define PUBNUB_ASSERT_IMPL(X) do {                                      \
        PUBNUB_ANALYSIS_ASSUME(X);										\
        (X) ? (void)0 : pubnub_assert_failed(#X, __FILE__, __LINE__);   \
    } while (false)

/** Should make the compiler not report "unused variable"
    warnings.
*/
#define PUBNUB_UNUSED(x) do { (void)sizeof(x); } while (false)


/* Define the ASSERT macro for the highest (_EX) level.
 */
#if defined PUBNUB_ASSERT_LEVEL_EX
#define PUBNUB_ASSERT_EX(X) PUBNUB_ASSERT_IMPL(X)
#else
#define PUBNUB_ASSERT_EX(X) PUBNUB_UNUSED(X)
#endif

/* Determine if regular level is to be used. 
 */
#if defined(PUBNUB_ASSERT_LEVEL_EX) || defined(PUBNUB_ASSERT_LEVEL)
#define PUBNUB_ASSERT_IS_ACTIVE       // also usable directly in client code
#endif

/* Define the ASSERT macro for the regular level.
 */
#if defined(PUBNUB_ASSERT_IS_ACTIVE)
#define PUBNUB_ASSERT(X) PUBNUB_ASSERT_IMPL(X)
#else
#define PUBNUB_ASSERT(X) PUBNUB_UNUSED(X)
#endif


/* Define the ASSERT macro for the lowest level
 */
#if !defined(PUBNUB_ASSERT_LEVEL_NONE)
#define PUBNUB_ASSERT_OPT(X) PUBNUB_ASSERT_IMPL(X)
#else
#define PUBNUB_ASSERT_OPT(X) PUBNUB_UNUSED(X)
#endif



/** This will invoke the installed assert handler.  The default
    behavior is pubnub_assert_handler_abort().
 */
void pubnub_assert_failed(char const *s, char const *file, long line);

/** Prototype of a Pubnub assertion failure handler. There are several
    standard handlers, but you can also provide your own.
    @param s The string that defines the failure condition
    @param file The name of the source file with the condition
    @param line Number of the line of @c file with the condition
 */
typedef void (*pubnub_assert_handler_t)(char const *s, char const *file, long line);

/** This will install an assert handler. It can be one of the standard
    ones, or your own.

    @param handler The handler to install. If NULL, will install
    pubnub_assert_handler_abort()
 */
void pubnub_assert_set_handler(pubnub_assert_handler_t handler);

/** This handler will print a message formed from the parameters and
    then go to infinite loop. Useful for debugging.
*/
void pubnub_assert_handler_loop(char const *s, char const *file, long line);

/** This handler will print a message  formed from the parameters and
    then abort (exit, end) the process. Useful for testing.
 */
void pubnub_assert_handler_abort(char const *s, char const *file, long line);

/** This handler will print a message formed from the parameters and
    that's it. Useful for getting data from a program execution "in
    the field", where we must not stop or crash because of ASSERT
    failure.
 */
void pubnub_assert_handler_printf(char const *s, char const *file, long line);


#define PUBNUB_CTASTR2(pre,post,lex) pre ## post ## lex
#define PUBNUB_CTASTR(pre,post,lex) PUBNUB_CTASTR2(pre,post,lex)

#define PUBNUB_STATIC_ASSERT(cond) \
    typedef struct { int PUBNUB_CTASTR(static_assertion_failed_,msg) : !!(cond); } \
        PUBNUB_CTASTR(static_assertion_failed_,__FILE__,__LINE__)


#endif /* !defined INC_PUBNUB_ASSERT */
