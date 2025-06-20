/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include <cstdint>
#include <stddef.h>
#include <stdint.h>
#if !defined INC_PUBNUB_COMMON_HPP
#define INC_PUBNUB_COMMON_HPP


#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "pubnub_config.h"
#include "lib/pb_deprecated.h"
#include "core/pubnub_alloc.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_generate_uuid.h"
#include "core/pubnub_blocking_io.h"
#include "core/pubnub_ssl.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_free_with_timeout.h"
#if defined(PUBNUB_CALLBACK_API)
#include "core/pubnub_ntf_callback.h"
#endif
#if PUBNUB_PROXY_API
#include "core/pubnub_proxy.h"
#endif
#if PUBNUB_USE_SUBSCRIBE_V2
#include "core/pubnub_subscribe_v2.h"
#endif
#if PUBNUB_CRYPTO_API
#include "core/pubnub_crypto.h"
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
#include "core/pubnub_advanced_history.h"
#endif
#if PUBNUB_USE_FETCH_HISTORY
#include "core/pubnub_fetch_history.h"
#endif
#if PUBNUB_USE_OBJECTS_API
#include "core/pubnub_objects_api.h"
#define MAX_INCLUDE_DIMENSION 100
#define MAX_ELEM_LENGTH 30
#endif
#if PUBNUB_USE_ACTIONS_API
#include "core/pubnub_actions_api.h"
#endif
#if PUBNUB_USE_GRANT_TOKEN_API
#include "core/pubnub_grant_token_api.h"
#endif
#if PUBNUB_USE_REVOKE_TOKEN_API
#include "core/pubnub_revoke_token_api.h"
#endif
#include "core/pubnub_auto_heartbeat.h"
#include "core/pubnub_crypto.h"
#include "core/pubnub_memory_block.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include "pubnub_mutex.hpp"
#include "tribool.hpp"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "pubnub_v2_message.hpp"
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <stdexcept>

#if __cplusplus >= 201103L
#include <chrono>
#endif

#include <iostream>


namespace pubnub {

/** An algorithm to join (contatenate) the elements of the given
    range [first, last) to a string-like object, separating
    elements with the @p separator.

    A more abstract way to think about this algorithm is
    "accumulate (left fold) with a separator between elements"
    that can be used on any object that has an `+=` operator.

    It is implied that the range is from a container of elements
    that have the same type as the @p result, or of types that can
    be added (via `+=`) to the type of the @p result.

    The result type only has to support the `+=` operator and be
    copy or move constructible.

    @param first Input iterator of the start of the range
    @param last Input iterator of the end of the range (one past the
    last element)
    @param separator The object to put (add) between elements
    in the result.
    @param result The object to which put (add) all the elements
    and the separator - the initial value.
    @return The final result, after all additions (if any)
 */
template <typename I, typename S>
S join(I first, I last, S const& separator, S result)
{
    if (first == last) {
        return result;
    }
    I prev = first;
    I next = first;
    for (++next; next != last; ++next) {
        result += *prev;
        result += separator;
        prev = next;
    }
    result += *prev;
    return result;
}

/** Pass a STL-like container for @p container, we'll do the join
    from container.begin() to container.end(). A helper function
    to use you know that you want to join all the elements of a
    container.
 */
template <typename C, typename S>
S join(C container, S const& separator, S result)
{
    return join(container.begin(), container.end(), separator, result);
}

static const std::string comma = ",";

/** We actually know we always join strings and commas, so, this
    helps simplify things.
 */
template <typename C> std::string join(C container)
{
    return join(container, comma, std::string());
}

/// Posible settings for (usage of) blocking I/O
enum blocking_io {
    /// Use blocking I/O
    blocking,
    /// Use non-blocking I/O
    non_blocking
};

/** Options for SSL/TLS transport. These are designed to be used as
 * "bit-masks", for which purpose there are overloaded `&` and `|`
 * (bit-and and bit-or) operators.
 */
enum ssl_opt {
    /// Should the PubNub client establish the connection to
    /// PubNub using SSL? (default: YES)
    useSSL = 0x01,
    /// When SSL is enabled, should the client fallback to a
    /// non-SSL connection if it experiences issues handshaking
    /// across local proxies, firewalls, etc? (default: YES)
    ignoreSecureConnectionRequirement = 0x02
};
inline ssl_opt operator|(ssl_opt l, ssl_opt r)
{
    return static_cast<ssl_opt>(static_cast<int>(l) | static_cast<int>(r));
}
inline ssl_opt operator&(ssl_opt l, ssl_opt r)
{
    return static_cast<ssl_opt>(static_cast<int>(l) & static_cast<int>(r));
}

#if PUBNUB_PROXY_API
/// Possible proxy types
enum proxy_type {
    /// HTTP GET proxy - the simplest
    http_get_proxy,
    /// HTTP CONNECT - tunnel proxy
    http_connect_proxy,
    /// No proxy at all
    none_proxy,
};

/// Return C-core proxy type from C++ (wrapper) proxy type
inline enum pubnub_proxy_type ccore_proxy_type(proxy_type prtp)
{
    switch (prtp) {
    case http_get_proxy:
        return pbproxyHTTP_GET;
    case http_connect_proxy:
        return pbproxyHTTP_CONNECT;
    default:
        return pbproxyNONE;
    }
}
#endif

/** A wrapper class for subscribe options, enabling a nicer
    usage. Something like:

        pn.subscribe(chan, subscribe_options().heartbeat(412));
*/
class subscribe_options {
    pubnub_subscribe_options d_;
    std::string              d_chgrp;

public:
    subscribe_options() { d_ = pubnub_subscribe_defopts(); }
    subscribe_options& channel_group(std::string const& chgroup)
    {
        d_chgrp          = chgroup;
        d_.channel_group = d_chgrp.empty() ? 0 : d_chgrp.c_str();
        return *this;
    }
    subscribe_options& channel_group(std::vector<std::string> const& chgroup)
    {
        return channel_group(join(chgroup));
    }
    subscribe_options& heartbeat(unsigned hb_interval)
    {
        d_.heartbeat = hb_interval;
        return *this;
    }
    pubnub_subscribe_options data() { return d_; }
};


#if PUBNUB_USE_SUBSCRIBE_V2
/** A wrapper class for subscribe_v2 options, enabling a nicer
    usage. Something like:

        pn.subscribe(chan, subscribe_v2_options().heartbeat(412));
*/
class subscribe_v2_options {
    pubnub_subscribe_v2_options d_;
    std::string d_chgrp;
    std::string d_filter_expr;
    
public:
    subscribe_v2_options() { d_ = pubnub_subscribe_v2_defopts(); }
    subscribe_v2_options& channel_group(std::string const& chgroup)
    {
        d_chgrp          = chgroup;
        d_.channel_group = d_chgrp.empty() ? 0 : d_chgrp.c_str();
        return *this;
    }
    subscribe_v2_options& channel_group(std::vector<std::string> const& chgroup)
    {
        return channel_group(join(chgroup));
    }
    subscribe_v2_options& heartbeat(unsigned hb_interval)
    {
        d_.heartbeat = hb_interval;
        return *this;
    }
    subscribe_v2_options& filter_expr(std::string const& filter_exp)
    {
        d_filter_expr = filter_exp;
        d_.filter_expr = d_filter_expr.empty() ? 0 : d_filter_expr.c_str();
        return *this;
    }
    pubnub_subscribe_v2_options data() { return d_; }
};
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

/** A wrapper class for publish options, enabling a nicer
    usage. Something like:

        pn.publish(chan, message, publish_options().store(false));
*/
class publish_options {
    pubnub_publish_options d_;
    std::string            d_ciphkey;
    std::string            d_mtdt;

public:
    /** Default options for publish via GET */
    publish_options() { d_ = pubnub_publish_defopts(); }
    publish_options& store(bool stor)
    {
        d_.store = stor;
        return *this;
    }
    publish_options& cipher_key(std::string ciphkey)
    {
        d_ciphkey     = ciphkey;
        d_.cipher_key = d_ciphkey.c_str();
        return *this;
    }
    publish_options& replicate(unsigned rplct)
    {
        d_.replicate = rplct;
        return *this;
    }
    publish_options& meta(std::string mtdt)
    {
        d_mtdt  = mtdt;
        d_.meta = d_mtdt.c_str();
        return *this;
    }
    publish_options& method(pubnub_method method)
    {
        d_.method = method;
        return *this;
    }
    pubnub_publish_options data() { return d_; }
};


/** A wrapper class for here-now options, enabling a nicer
    usage. Something like:

        pn.here_now(chan, here_now_options().heartbeat(412));
*/
class here_now_options {
    pubnub_here_now_options d_;
    std::string             d_chgrp;

public:
    here_now_options() { d_ = pubnub_here_now_defopts(); }
    here_now_options& channel_group(std::string const& chgroup)
    {
        d_chgrp          = chgroup;
        d_.channel_group = d_chgrp.empty() ? 0 : d_chgrp.c_str();
        return *this;
    }
    here_now_options& channel_group(std::vector<std::string> const& chgroup)
    {
        return channel_group(join(chgroup));
    }
    here_now_options& disable_uuids(bool disable_uuids)
    {
        d_.disable_uuids = disable_uuids;
        return *this;
    }
    here_now_options& state(bool state)
    {
        d_.state = state;
        return *this;
    }
    pubnub_here_now_options data() { return d_; }
};

/** A wrapper class for history options, enabling a nicer
    usage. Something like:

        pn.history(chan, history_options().reverse(true));
*/
class history_options {
    pubnub_history_options d_;
    std::string            d_strt;
    std::string            d_ender;

public:
    history_options() { d_ = pubnub_history_defopts(); }
    history_options& string_token(bool token_string)
    {
        d_.string_token = token_string;
        return *this;
    }
    history_options& count(int cnt)
    {
        d_.count = cnt;
        return *this;
    }
    history_options& reverse(bool rev)
    {
        d_.reverse = rev;
        return *this;
    }
    history_options& start(std::string const& st)
    {
        d_strt   = st;
        d_.start = d_strt.empty() ? 0 : d_strt.c_str();
        return *this;
    }
    history_options& end(std::string const& e)
    {
        d_ender = e;
        d_.end  = d_ender.empty() ? 0 : d_ender.c_str();
        return *this;
    }
    history_options& include_token(bool inc_token)
    {
        d_.include_token = inc_token;
        return *this;
    }
    history_options& include_meta(bool inc_meta)
    {
        d_.include_meta = inc_meta;
        return *this;
    }
    pubnub_history_options data() { return d_; }
};

/** A wrapper class for set_state options, enabling a nicer
    usage. Something like:

        pn.set_state(chan, state, pubnub::set_state_options().heartbeat(true));
*/

class set_state_options {
    pubnub_set_state_options d_;
    std::string d_channel_group;
    std::string d_user_id;

public:
    set_state_options() : d_(pubnub_set_state_options()) {}

    set_state_options& channel_group(std::string const& channel_group)
    {
        d_channel_group = channel_group;
        d_.channel_group = d_channel_group.empty() ? 0 : d_channel_group.c_str();

        return *this;
    }

    set_state_options& channel_group(std::vector<std::string> const& channel_groups)
    {
        return channel_group(join(channel_groups));
    }

    set_state_options& user_id(std::string const& user_id)
    {
        d_user_id = user_id;
        d_.user_id = d_user_id.empty() ? 0 : d_user_id.c_str();

        return *this;
    }

    set_state_options& heartbeat(bool heartbeat)
    {
        d_.heartbeat = heartbeat;

        return *this;
    }

    pubnub_set_state_options data() { return d_; }
};

#if PUBNUB_USE_OBJECTS_API
/** A wrapper class for objects api managing include parameter */
class include_options {
    char d_include_c_strings_array[MAX_INCLUDE_DIMENSION][MAX_ELEM_LENGTH + 1];
    size_t d_include_count;
    
public:
    include_options()
        : d_include_count(0)
    {}
    char const** include_c_strings_array()
    {
        return (d_include_count > 0) ? (char const**)d_include_c_strings_array : NULL;
    }
    char const** include_to_c_strings_array(std::vector<std::string> const& inc)
    {
        size_t n = inc.size();
        unsigned i;
        if (n > MAX_INCLUDE_DIMENSION) {
            throw std::range_error("include parameter has too many elements.");
        }
        for (i = 0; i < n; i++) {
            if (inc[i].size() > MAX_ELEM_LENGTH) {
                throw std::range_error("include string element is too long.");
            }
            strcpy(d_include_c_strings_array[i], inc[i].c_str());
        }
        d_include_count = n;
        return include_c_strings_array();
    }
    include_options(std::vector<std::string> const& inc)
    {
        include_to_c_strings_array(inc);
    }
    size_t include_count() { return d_include_count; }
};
    
/** A wrapper class for objects api paging option parameters, enabling a nicer
    usage. Something like:
       pbp.get_users(list_options().start(last_bookmark));

    instead of:
       pbp.get_users(nullopt, nullopt, last_bookmark, “”, nullopt);
  */
class list_options {
    std::string d_include;
    size_t d_limit;
    std::string d_start;
    std::string d_end;
    tribool d_count;

public:
    list_options()
        : d_limit(0)
        , d_count(tribool::not_set)
    {}
    list_options& include(std::string const& incl)
    {
        d_include = incl;
        return *this;
    }
    char const* include() { return (d_include.size() > 0) ? d_include.c_str() : NULL; }

    list_options& limit(size_t lim)
    {
        d_limit = lim;
        return *this;
    }
    size_t limit() { return d_limit; }
    list_options& start(std::string const& st)
    {
        d_start = st;
        return *this;
    }
    char const* start() { return (d_start.size() > 0) ? d_start.c_str() : NULL; }
    list_options& end(std::string const& e)
    {
        d_end = e;
        return *this;
    }
    char const* end() { return (d_end.size() > 0) ? d_end.c_str() : NULL; }
    list_options& count(tribool co)
    {
        d_count = co;
        return *this;
    }
    pubnub_tribool count()
    {
        if (false == d_count) {
            return pbccFalse;
        }
        else if (true == d_count) {
            return pbccTrue;
        }
        return pbccNotSet;
    }
};
#endif /* PUBNUB_USE_OBJECTS_API */    

#if PUBNUB_CRYPTO_API
/** Interface for a cryptor. It is an algorithm class that
 * provides encryption and decryption of arrays of bytes.
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * Note that name comes from Rust, where it is used to convert from type to type.
 * The pointer returned by `into_cryptor_ptr()` should be allocated with `new`.
 *
 * @see pubnub_cryptor 
*/
class into_cryptor_ptr {
public:
    ~into_cryptor_ptr() { }

    virtual pubnub_cryptor_t* into_cryptor() = 0;
};


/** Interface for a crypto provider. It is used to encrypt and decrypt 
 * messages sent and received from Pubnub. It makes a conversion from 
 * your cryptographic module to the C interface used by the C++ Pubnub context.
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * Note that name comes from Rust, where it is used to convert from type to type.
 * The pointer returned by `into_provider_ptr()` should be allocated with `new`.
 *
 * @see pubnub_crypto_provider_t
*/
class into_crypto_provider_ptr {
public:
    ~into_crypto_provider_ptr() { }

    virtual pubnub_crypto_provider_t* into_provider() = 0;
};


/** Default implementation of the cryptoprovider.
 *
 * It implements the "into_crypto_provider_ptr" interface. 
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * It is designed to behave like the default PubNub crypto module 
 * returned by `pubnub_crypto_aes_cbc_module_init()`,
 * `pubnub_crypto_legacy_module_init()` and `pubnub_crypto_module_init()` functions.
 *
 * @see crypto_provider
 * @see pubnub_crypto_aes_cbc_module_init
 * @see pubnub_crypto_legacy_module_init
 * @see pubnub_crypto_module_init
*/
class crypto_module: public into_crypto_provider_ptr {
public:
    /* Wrapping constructor for the `pubnub_crypto_provider_t` structure.
     *
     * Note that this function takes ownership of the `crypto_module` pointer.
     * It will be freed when the PubNub `context` object is destroyed.
     *
     * @param crypto_module The `pubnub_crypto_provider_t` structure to wrap.
     */
    crypto_module(pubnub_crypto_provider_t* crypto_module) : d_module(crypto_module) {}

    /* Constructor that translates C++ data into the `pubnub_crypto_provider_init` function.
     *
     * Note that this function takes vector of `into_cryptor_ptr` data, that transletes
     * into the `pubnub_cryptor_t` structure.
     * It copies the data from the vector into the `additional_cryptors` vector and
     * allocates the new C array of `pubnub_cryptor_t` structures.
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap.
     */
    crypto_module(into_cryptor_ptr &default_cryptor, std::vector<into_cryptor_ptr*> &additional_cryptors)
    {
        size_t size = additional_cryptors.size();
        pubnub_cryptor_t* cryptors = new pubnub_cryptor_t[sizeof(pubnub_cryptor_t*) * size];

        for (size_t i = 0; i < size; ++i) {
            pubnub_cryptor_t* cryptor = additional_cryptors[i]->into_cryptor();
            cryptors[i] = *cryptor;
        }

        this->d_module = pubnub_crypto_module_init(default_cryptor.into_cryptor(), cryptors, size);
    }

    /* Constructor that creates C++ `crypto_module` object that mimics
     * the `pubnub_crypto_aes_cbc_module_init` function. 
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap.(it doesn't own the key - keep it alive)
     *
     * @return The `crypto_module` object
     *
     * @see pubnub_crypto_aes_cbc_module_init
     */
    static crypto_module aes_cbc(std::string& cipher_key)
    {
        return crypto_module(pubnub_crypto_aes_cbc_module_init((uint8_t*)(cipher_key.c_str())));
    }

    /* Constructor that creates C++ `crypto_module` object that mimics
     * the `pubnub_crypto_legacy_module_init` function. 
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap. (it doesn't own the key - keep it alive)
     *
     * @return The `crypto_module` object
     *
     * @see pubnub_crypto_legacy_module_init
     */
    static crypto_module legacy(std::string& cipher_key)
    {
        return crypto_module(pubnub_crypto_legacy_module_init((uint8_t*)(cipher_key.c_str())));
    }

    /* Encrypts the @p to_encrypt buffer and returns the encrypted
     * buffer.
     *
     * @param to_encrypt The buffer to encrypt
     *
     * @return The encrypted buffer
     */
    std::vector<std::uint8_t> encrypt(std::vector<std::uint8_t>& to_encrypt) {
        pubnub_bymebl_t to_encrypt_c;
        to_encrypt_c.ptr = to_encrypt.data();
        to_encrypt_c.size = to_encrypt.size();

        pubnub_bymebl_t result = this->d_module->encrypt(this->d_module, to_encrypt_c);

        if (result.ptr == nullptr) {
            throw std::runtime_error("crypto_module::encrypt: Encryption failed!");
        }

        return std::vector<std::uint8_t>(result.ptr, result.ptr + result.size);
    }

    /* Decrypts the @p to_decrypt buffer and returns the decrypted
     * buffer.
     *
     * @param to_decrypt The buffer to decrypt
     *
     * @return The decrypted buffer
     */
    std::vector<std::uint8_t> decrypt(std::vector<std::uint8_t>& to_decrypt) {
        pubnub_bymebl_t to_decrypt_c;
        to_decrypt_c.ptr = to_decrypt.data();
        to_decrypt_c.size = to_decrypt.size();

        pubnub_bymebl_t result = this->d_module->decrypt(this->d_module, to_decrypt_c);

        if (result.ptr == nullptr) {
            throw std::runtime_error("crypto_module::decrypt: Decryption failed!");
        }

        return std::vector<std::uint8_t>(result.ptr, result.ptr + result.size);
    }

    pubnub_crypto_provider_t* into_provider() {
        return this->d_module;
    }

private:
    pubnub_crypto_provider_t* d_module;
};
#endif // #if PUBNUB_CRYPTO_API

/** The C++ Pubnub context. It is a wrapper of the Pubnub C context,
 * not a "native" C++ implementation.
 *
 * One of its main design decisions is to make it safe,
 * sacrificing some performance (and memory footprint). The
 * rationale is that you can use the Pubnub C client "as-is" from
 * C++ if you're going for really high performance and low
 * footprint.
 *
 * The other main design decision is to use a "std::future<>" like
 * interface for getting the outcome of a Pubnub transaction and
 * that is the same regardless of the interface used for the
 * Pubnub C library that we wrap. IOW, it is the same for both the
 * "sync" and "callback" interfaces. Thus, C++ client code is
 * always the same and the user selects during build the Pubnub C
 * "backend" to use.
 *
 * Also, here we document the C++ aspects of certain methods and
 * their general meaning. For details, refer to the Pubnub C
 * documentation, as this is just a wrapper, all of it holds.
 */
class context {
public:
    /** Create the context, that will use @p pubkey as its
        publish key and @subkey as its subscribe key for its
        lifetime. These cannot be changed, to use another
        pair, create a new context.
        @see pubnub_alloc, pubnub_init
    */
    context(std::string pubkey, std::string subkey)
        : d_pubk(pubkey)
        , d_ksub(subkey)
    {
        pubnub_mutex_init(d_mutex);
        d_pb = pubnub_alloc();
        if (0 == d_pb) {
            throw std::bad_alloc();
        }
        pubnub_init(d_pb, d_pubk.c_str(), d_ksub.c_str());
    }

    context(std::string pubkey, std::string subkey, std::string origin)
        : d_pubk(pubkey)
        , d_ksub(subkey)
    {
        pubnub_mutex_init(d_mutex);
        d_pb = pubnub_alloc();
        if (0 == d_pb) {
            throw std::bad_alloc();
        }
        pubnub_init(d_pb, d_pubk.c_str(), d_ksub.c_str());
        set_origin(origin);
    }

    /** Sets the origin to @p origin on this context. This may fail.
        To reset to default, pass an empty string.
        @see pubnub_origin_set
     */
    int set_origin(std::string const& origin)
    {
        lock_guard lck(d_mutex);
        d_origin = origin;
        return pubnub_origin_set(d_pb, origin.empty() ? NULL : d_origin.c_str());
    }
    /** Returns the current origin for this context
        @see pubnub_get_origin
     */
    std::string origin() const { return pubnub_get_origin(d_pb); }


    /** Sets the port to @p port on this context. This may fail.
        @see pubnub_port_set
     */
    int set_port(uint16_t port)
    {
        lock_guard lck(d_mutex);
        return pubnub_port_set(d_pb, port);
    }

    /** Sets the secret key to @p secret_key. If @p secret_key is an 
        empty string, `secret_key` will not be used.
        @see pubnub_set_secret_key
     */
    void set_secret_key(std::string const& secret_key)
    {
        lock_guard lck(d_mutex);
        if (!pubnub_can_start_transaction(d_pb)) {
            throw std::logic_error("setting 'secret_key' key while transaction in progress");
        }
        d_secret_key = secret_key;
        #if PUBNUB_CRYPTO_API
        pubnub_set_secret_key(d_pb, secret_key.empty() ? NULL : d_secret_key.c_str());
        #else
        throw std::logic_error("PUBNUB_CRYPTO_API is not enabled to use 'secret_key'");
        #endif
    }

    /// Returns the current `secret_key` for this context
    std::string const& secret_key() const
    {
        lock_guard lck(d_mutex);
        return d_secret_key;
    }

    /** Sets the `auth` key to @p auth. If @p auth is an
        empty string, `auth` will not be used.
        @see pubnub_set_auth
     */
    void set_auth(std::string const& auth)
    {
        lock_guard lck(d_mutex);
        if (!pubnub_can_start_transaction(d_pb)) {
            throw std::logic_error("setting 'auth' key while transaction in progress");
        }
        d_auth = auth;
        pubnub_set_auth(d_pb, auth.empty() ? NULL : d_auth.c_str());
    }
    /// Returns the current `auth` key for this context
    std::string const& auth() const
    {
        lock_guard lck(d_mutex);
        return d_auth;
    }

    /** Sets the auth `token` to @p token. If @p token is an
        empty string, `token` will not be used.
        @see pubnub_set_auth_token
     */
    void set_auth_token(std::string const& token)
    {
        lock_guard lck(d_mutex);
        if (!pubnub_can_start_transaction(d_pb)) {
            throw std::logic_error("setting 'auth_token' while transaction in progress");
        }
        d_auth_token = token;
        pubnub_set_auth_token(d_pb, token.empty() ? NULL : d_auth_token.c_str());
    }
    /// Returns the current `auth_token` for this context
    std::string const& auth_token() const
    {
        lock_guard lck(d_mutex);
        return d_auth_token;
    }
    
    /** sets the user_id to @p uuid. if @p uuid is an empty string,
      user_id will not be used.

      @deprecated this is provided as a workaround for existing users.
      Please use `set_user_id` instead.

      @see pubnub_set_user_id
      */
    PUBNUB_DEPRECATED void set_uuid(std::string const& uuid)
    {
        set_user_id(uuid);
    }

    /** sets the user_id to @p user_id. if @p user_id is an empty string,
      user_id will not be used.

      @see pubnub_set_user_id
      */
    void set_user_id(std::string const& user_id)
    {
        pubnub_set_user_id(d_pb, user_id.empty() ? NULL : user_id.c_str());
    }

    /// Set the user_id with a random-generated UUID
    /// 
    /// @deprecated random generated uuid/user_id is deprecated.
    ///
    /// @see pubnub_generate_uuid_v4_random
    PUBNUB_DEPRECATED int set_uuid_v4_random() 
    {
        struct Pubnub_UUID uuid;
        if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
            return -1;
        }
        set_user_id(pubnub_uuid_to_string(&uuid).uuid);
        return 0;
    }

    /** Returns the current user_id

      @deprecated this is provided as a workaround for existing users.
      Please use `user_id` instead.
      */
    PUBNUB_DEPRECATED std::string const uuid() const
    {
        return user_id();
    }

    /// Returns the current user_id
    std::string const user_id() const
    {
        char const* uuid = pubnub_user_id_get(d_pb);
        return std::string((NULL == uuid) ? "" : uuid);
    }

    /// Returns the next message from the context. If there are
    /// none, returns an empty string.
    /// @see pubnub_get
    std::string get() const
    {
        char const* msg = pubnub_get(d_pb);
        return (NULL == msg) ? "" : msg;
    }
    /// Returns a vector of all messages from the context.
    std::vector<std::string> get_all() const
    {
        std::vector<std::string> all;
        while (char const* msg = pubnub_get(d_pb)) {
            if (NULL == msg) {
                break;
            }
            all.push_back(msg);
        }
        return all;
    }

#if PUBNUB_USE_SUBSCRIBE_V2
    /// Returns the next v2 message from the context. If there are
    /// none, returns an empty message structure(checked through
    /// v2_mesage::is_empty()). 
    /// @see pubnub_get_v2
    v2_message get_v2() const
    {
        return v2_message(pubnub_get_v2(d_pb));
    }
    /// Returns a vector of all v2 messages from the context.
    std::vector<v2_message> get_all_v2() const
    {
        std::vector<v2_message> all;
        v2_message msg = get_v2();

        while (!msg.is_empty()) {
            all.push_back(msg);
            msg = get_v2();
        }
        return all;
    }
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

#if PUBNUB_CRYPTO_API
    /// Returns the next message from the context, decrypted with
    /// @p cipher_key. If there are none, returns an empty string.
    /// If decrypting fails, also returns an empty string.
    /// @see pubnub_get
    std::string get_decrypted(std::string const& cipher_key) const
    {
        pubnub_bymebl_t mebl =
            pubnub_get_decrypted_alloc(d_pb, cipher_key.c_str());
        if (NULL == mebl.ptr) {
            return "";
        }
        std::string rslt(reinterpret_cast<char*>(mebl.ptr));
        ::free(mebl.ptr);
        return rslt;
    }
    /// Returns the all the remaining messages from the context,
    /// all decrypted with the same @p cipher_key. If there are
    /// none, returns an empty vector.  If decrypting of any
    /// message fails, it will not be in the returned vector.
    std::vector<std::string> get_all_decrypted(std::string const& cipher_key) const
    {
        std::vector<std::string> all;
        for (;;) {
            pubnub_bymebl_t mebl =
                pubnub_get_decrypted_alloc(d_pb, cipher_key.c_str());
            if (NULL == mebl.ptr) {
                break;
            }
            all.push_back(reinterpret_cast<char*>(mebl.ptr));
            ::free(mebl.ptr);
        }
        return all;
    }
#endif

    /// Returns the next channel string from the context.
    /// If there are none, returns an empty string
    /// @see pubnub_get_channel
    std::string get_channel() const
    {
        char const* chan = pubnub_get_channel(d_pb);
        return (NULL == chan) ? "" : chan;
    }
    /// Returns a vector of all channel strings from the context
    /// @see pubnub_get_channel
    std::vector<std::string> get_all_channels() const
    {
        std::vector<std::string> all;
        while (char const* msg = pubnub_get_channel(d_pb)) {
            if (NULL == msg) {
                break;
            }
            all.push_back(msg);
        }
        return all;
    }

    /// Cancels the transaction, if any is ongoing, or connection is 'kept alive'. 
    /// If none is ongoing, it is ignored.
    /// @see pubnub_cancel
    enum pubnub_cancel_res cancel()
    {
        return pubnub_cancel(d_pb);
    }

    /// Publishes a @p message on the @p channel. The @p channel
    /// can have many channels separated by a comma
    /// @see pubnub_publish
    inline futres publish(std::string const& channel, std::string const& message)
    {
        return doit(pubnub_publish(d_pb, channel.c_str(), message.c_str()));
    }

    /// Publishes a @p message on the @p channel with "extended"
    /// (full) options.
    futres publish(std::string const& channel,
                   std::string const& message,
                   publish_options    opt)
    {
        return doit(pubnub_publish_ex(d_pb, channel.c_str(), message.c_str(), opt.data()));
    }

    /// Sends a signal @p message on the @p channel.
    /// @see pubnub_signal
    futres signal(std::string const& channel, std::string const& message)
    {
        return doit(pubnub_signal(d_pb, channel.c_str(), message.c_str()));
    }
    
#if PUBNUB_CRYPTO_API
    /// Publishes a @p message on the @p channel encrypted with @p
    /// cipher_key.
    /// @see pubnub_publish_encrypted
    futres publish_encrypted(std::string const& channel,
                             std::string const& message,
                             std::string const& cipher_key)
    {
        return doit(pubnub_publish_encrypted(
            d_pb, channel.c_str(), message.c_str(), cipher_key.c_str()));
    }
#endif

    /// Subscribes to @p channel and/or @p channel_group
    /// @see pubnub_subscribe
    futres subscribe(std::string const& channel,
                     std::string const& channel_group = "")
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(pubnub_subscribe(d_pb, ch, gr));
    }

    /// Pass a vector of channels in the @p channel and a vector
    /// of channel groups for @p channel_group and we will put
    /// commas between them. A helper function.
    futres subscribe(std::vector<std::string> const& channel,
                     std::vector<std::string> const& channel_group)
    {
        return subscribe(join(channel), join(channel_group));
    }

    /// Subscribes to @p channel with "extended" (full) options
    /// @see pubnub_subscribe_ex
    futres subscribe(std::string const& channel, subscribe_options opt)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        return doit(pubnub_subscribe_ex(d_pb, ch, opt.data()));
    }

    /// Pass a vector of channels in the @p channel and we'll put
    /// commas between them. A helper function.
    futres subscribe(std::vector<std::string> const& channel, subscribe_options opt)
    {
        return subscribe(join(channel), opt);
    }

#if PUBNUB_USE_SUBSCRIBE_V2
    /// V2 subscribes to @p channel with "extended" (full) options
    /// @see pubnub_subscribe_v2
    futres subscribe_v2(std::string const& channel, subscribe_v2_options opt)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        return doit(pubnub_subscribe_v2(d_pb, ch, opt.data()));
    }

    /// Pass a vector of channels in the @p channel and we'll put
    /// commas between them. A helper function.
    futres subscribe_v2(std::vector<std::string> const& channel, subscribe_v2_options opt)
    {
        return subscribe_v2(join(channel), opt);
    }
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

    /// Leaves a @p channel and/or @p channel_group
    /// @see pubnub_leave
    futres leave(std::string const& channel, std::string const& channel_group)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(pubnub_leave(d_pb, ch, gr));
    }

    /// Pass a vector of channels in the @p channel and a vector
    /// of channel groups for @p channel_group and we will put
    /// commas between them. A helper function.
    futres leave(std::vector<std::string> const& channel,
                 std::vector<std::string> const& channel_group)
    {
        return leave(join(channel), join(channel_group));
    }

    /// Starts a "get time" transaction
    /// @see pubnub_time
    futres time()
    {
        return doit(pubnub_time(d_pb));
    }

    /// Starts a transaction to get message history for @p channel
    /// with the limit of max @p count
    /// messages to retrieve, and optionally @p include_token to get
    /// a time token for each message.
    /// @see pubnub_history
    futres history(std::string const& channel,
                   unsigned           count         = 100,
                   bool               include_token = false)
    {
        return doit(pubnub_history(d_pb,
                                   channel.empty() ? 0 : channel.c_str(),
                                   count,
                                   include_token));
    }

    /// Starts a "history" with extended (full) options
    /// @see pubnub_history_ex
    futres history(std::string const& channel, history_options opt)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        return doit(pubnub_history_ex(d_pb, ch, opt.data()));
    }

    /// In case the server reported en error in the response,
    /// we'll read the error message using this function
    /// @retval error_message on successfully read error message,
    /// @retval empty_string otherwise
    std::string get_error_message()
    {
        pubnub_chamebl_t msg;
        if (pubnub_get_error_message(d_pb, &msg) != 0) {
            return std::string("");
        }
        return std::string(msg.ptr, msg.size);
    }

    /// In case the server reported en error in the response,
    /// we'll read the error message using this function
    /// @retval error_message on successfully read error message,
    /// @retval empty_string otherwise
    std::string last_http_response_body()
    {
        pubnub_chamebl_t msg;
        if (pubnub_last_http_response_body(d_pb, &msg) != 0) {
            return std::string("");
        }
        return std::string(msg.ptr, msg.size);
    }

    /// Starts 'advanced history' pubnub_message_counts operation
    /// for unread messages on @p channel(channel list) starting from
    /// the given @p timetoken
    futres message_counts(std::string const& channel, std::string const& timetoken)
    {
        return doit(pubnub_message_counts(d_pb,
                                          channel.empty() ? 0 : channel.c_str(),
                                          timetoken.empty() ? 0 : timetoken.c_str()));
    }
    
    /// Starts 'advanced history' pubnub_message_counts operation
    /// for unread messages on @p channel(channel list) starting from
    /// the given @p timetoken
    futres message_counts(std::vector<std::string> const& channel,
                          std::string const& timetoken)
    {
        return message_counts(join(channel), timetoken);
    }
    
    /// Starts 'advanced history' pubnub_message_counts operation
    /// for unread messages on @p channel(channel list) starting from
    /// the given @p channel_timetokens(per channel)
    futres message_counts(std::string const& channel,
                          std::vector<std::string> const& channel_timetokens)
    {
        return message_counts(channel, join(channel_timetokens));
    }
    
    /// Starts 'advanced history' pubnub_message_counts operation
    /// for unread messages on @p channel(channel list) starting from
    /// the given @p channel_timetokens(per channel)
    futres message_counts(std::vector<std::string> const& channel,
                          std::vector<std::string> const& channel_timetokens)
    {
        return message_counts(join(channel), channel_timetokens);
    }
    
    /// Starts 'advanced history' pubnub_message_counts operation
    /// for unread messages on @p channel_timetokens(channel, ch_timetoken pairs)
    futres message_counts(
        std::vector<std::pair<std::string, std::string> > const& channel_timetokens)
    {
        std::string ch_list("");
        std::string tt_list("");
        unsigned    n = channel_timetokens.empty() ? 0 : channel_timetokens.size(); 
        unsigned    i;
        for (i = 0; i < n; i++) {
            std::string separator = ((i+1) < n) ? "," : "";
            ch_list += channel_timetokens[i].first + separator;
            tt_list += channel_timetokens[i].second + separator;
        }
        return message_counts(ch_list, tt_list);
    }

    /// Extracts channel-message_count paired map from the response on
    /// 'advanced history' pubnub_message_counts operation
    std::map<std::string, size_t> get_channel_message_counts()
    {
        std::map<std::string, size_t> map;
        std::vector<pubnub_chan_msg_count> chan_msg_counters;
        int i;
        int count = pubnub_get_chan_msg_counts_size(d_pb);
        if (count <= 0) {
            return map;
        }
        chan_msg_counters = std::vector<pubnub_chan_msg_count>(count);
        if (pubnub_get_chan_msg_counts(d_pb, (size_t*)&count, &chan_msg_counters[0]) != 0) {
            return map;
        }
        for (i = 0; i < count; i++) {
            map.insert(std::make_pair(std::string(chan_msg_counters[i].channel.ptr,
                                                  chan_msg_counters[i].channel.size),
                                      chan_msg_counters[i].message_count));
        }
        return map;
    }
    
    /// Starts a transaction to inform Pubnub we're working
    /// on a @p channel and/or @p channel_group
    /// @see pubnub_heartbeat
    futres heartbeat(std::string const& channel,
                     std::string const& channel_group = "")
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(pubnub_heartbeat(d_pb, ch, gr));
    }

    /// Pass a vector of channels in the @p channel and a vector
    /// of channel groups for @p channel_group and we will put
    /// commas between them. A helper function.
    futres heartbeat(std::vector<std::string> const& channel,
                     std::vector<std::string> const& channel_group)
    {
        return heartbeat(join(channel), join(channel_group));
    }

    /// Starts a transaction to get a list of currently present
    /// UUIDs on a @p channel and/or @p channel_group
    /// @see pubnub_here_now
    futres here_now(std::string const& channel, std::string const& channel_group = "")
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(pubnub_here_now(d_pb, ch, gr));
    }

    /// Pass a vector of channels in the @p channel and a vector
    /// of channel groups for @p channel_group and we will put
    /// commas between them. A helper function.
    futres here_now(std::vector<std::string> const& channel,
                    std::vector<std::string> const& channel_group)
    {
        return here_now(join(channel), join(channel_group));
    }

    /// Starts a "here-now" with extended (full) options
    /// @see pubnub_here_now_ex
    futres here_now(std::string const& channel, here_now_options opt)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        return doit(pubnub_here_now_ex(d_pb, ch, opt.data()));
    }

    /// Pass a vector of channels in the @p channel and we'll put
    /// commas between them. A helper function.
    futres here_now(std::vector<std::string> const& channel, here_now_options opt)
    {
        return here_now(join(channel), opt);
    }

    /// Starts a transaction to get a list of currently present
    /// UUIDs on all channels
    /// @see pubnub_global_here_now
    futres global_here_now()
    {
        return doit(pubnub_global_here_now(d_pb));
    }

    /// Starts a transaction to get a list of channels the @p uuid
    /// is currently present on. If @p uuid is not given (or is an
    /// empty string) it will
    /// @see pubnub_where_now
    futres where_now(std::string const& uuid = "")
    {
        return doit(pubnub_where_now(d_pb, uuid.empty() ? NULL : uuid.c_str()));
    }

    /// Starts a transaction to set the @p state JSON object for the
    /// given @p channel and/or @pchannel_group of the given @p uuid
    /// @see pubnub_set_state
    futres set_state(std::string const& channel,
                     std::string const& channel_group,
                     std::string const& uuid,
                     std::string const& state)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(pubnub_set_state(d_pb, ch, gr, uuid.c_str(), state.c_str()));
    }

    /// Pass a vector of channels in the @p channel and a vector
    /// of channel groups for @p channel_group and we will put
    /// commas between them. A helper function.
    futres set_state(std::vector<std::string> const& channel,
                     std::vector<std::string> const& channel_group,
                     std::string const&              uuid,
                     std::string const&              state)
    {
        return set_state(join(channel), join(channel_group), uuid, state);
    }

    /// Starts a transaction to set the @p state JSON object with selected 
    /// @options for the given @p channel and/or channel groups chosen 
    /// in options.
    /// @see pubnub_set_state_ex
    futres set_state(std::string const& channel,
                     std::string const& state,
                     set_state_options options)
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        return doit(pubnub_set_state_ex(d_pb, ch, state.c_str(), options.data()));
    }
    
    /// Starts a transaction to set the @p state JSON object with selected 
    /// @options for the given @p channels passed as a vector and/or channel
    /// groups chosen in options.
    /// @see pubnub_set_state_ex
    futres set_state(std::vector<std::string> const& channel,
                     std::string const& state,
                     set_state_options options)
    {
        return set_state(join(channel), state, options);
    }


    /// Starts a transaction to get the state JSON object for the
    /// given @p channel and/or @p channel_group of the given @p
    /// uuid.
    /// @see pubnub_state_get
    futres state_get(std::string const& channel,
                     std::string const& channel_group = "",
                     std::string const& uuid          = "")
    {
        char const* ch = channel.empty() ? 0 : channel.c_str();
        char const* gr = channel_group.empty() ? 0 : channel_group.c_str();
        return doit(
            pubnub_state_get(d_pb, ch, gr, uuid.empty() ? NULL : uuid.c_str()));
    }
    futres state_get(std::vector<std::string> const& channel,
                     std::vector<std::string> const& channel_group,
                     std::string const&              uuid = "")
    {
        return state_get(join(channel), join(channel_group), uuid);
    }

    /// Starts a transaction to remove a @p channel_group.
    /// @see pubnub_remove_channel_group
    futres remove_channel_group(std::string const& channel_group)
    {
        return doit(pubnub_remove_channel_group(d_pb, channel_group.c_str()));
    }

    /// Starts a transaction to remove a @p channel from a @p channel_group.
    /// @see pubnub_remove_channel_from_group
    futres remove_channel_from_group(std::string const& channel,
                                     std::string const& channel_group)
    {
        return doit(pubnub_remove_channel_from_group(
            d_pb, channel.c_str(), channel_group.c_str()));
    }

    /// Pass a vector of channels in the @p channel and we will
    /// put commas between them. A helper function.
    futres remove_channel_from_group(std::vector<std::string> const& channel,
                                     std::string const& channel_group)
    {
        return remove_channel_from_group(join(channel), channel_group);
    }

    /// Starts a transaction to add a @p channel to a @p channel_group.
    /// @see pubnub_add_channel_to_group
    futres add_channel_to_group(std::string const& channel,
                                std::string const& channel_group)
    {
        return doit(pubnub_add_channel_to_group(
            d_pb, channel.c_str(), channel_group.c_str()));
    }

    /// Pass a vector of channels in the @p channel and we will
    /// put commas between them. A helper function.
    futres add_channel_to_group(std::vector<std::string> const& channel,
                                std::string const&              channel_group)
    {
        return add_channel_to_group(join(channel), channel_group);
    }

    /// Starts a transaction to get a list of channels belonging
    /// to a @p channel_group.
    /// @see pubnub_list_channel_group
    futres list_channel_group(std::string const& channel_group)
    {
        return doit(pubnub_list_channel_group(d_pb, channel_group.c_str()));
    }

#if PUBNUB_USE_GRANT_TOKEN_API
    /// Starts a transaction that grants a token with the attributes specified in @p grant_obj.
    /// @see pubnub_grant_token
    futres grant_token(std::string const& grant_obj)
    {
        return doit(pubnub_grant_token(
                        d_pb,
                        grant_obj.c_str()));
    }

     /// Returns grant token
    /// @see pubnub_get_grant_token()
    std::string get_grant_token()
    {
        pubnub_chamebl_t result = pubnub_get_grant_token(d_pb);
        return std::string(result.ptr, result.size);
    }

    /// Parses the @p token and returns the json string.
    /// @see pubnub_grant_token
    std::string parse_token(std::string const& token)
    {
    	char* parsed_token_chars = pubnub_parse_token(d_pb, token.c_str());
        std::string owned_parsed_token(parsed_token_chars);
    	
    	::free(parsed_token_chars);
    	return owned_parsed_token; 
    }
#endif

#if PUBNUB_USE_REVOKE_TOKEN_API
    futres revoke_token(std::string const& token)
    {
        return doit(
            pubnub_revoke_token(d_pb, token.c_str())
        );
    }

    std::string get_revoke_token_result()
    {
        pubnub_chamebl_t result = pubnub_get_revoke_token_response(d_pb);
        return std::string(result.ptr, result.size);
    }

#endif

#if PUBNUB_USE_OBJECTS_API
    /// Starts a transaction for optaining a paginated list of uuids associated
    /// with the subscription key.
    /// @see pubnub_getall_uuidmetadata
    futres getall_uuidmetadata(list_options& options)
    {
        return doit(pubnub_getall_uuidmetadata(
                        d_pb, 
                        options.include(),
                        options.limit(),
                        options.start(),
                        options.end(),
                        options.count()));
    }

    /// Starts a transaction that creates a user with the attributes specified in @p user_obj.
    /// @see pubnub_set_uuidmetadata
    futres set_uuidmetadata(std::string const& uuid_metadataid, std::string const& include, std::string const& user_obj)
    {
        return doit(pubnub_set_uuidmetadata(
                        d_pb,
                        uuid_metadataid.c_str(),
                        include.c_str(),
                        user_obj.c_str()));
    }

    /// Starts a transaction that returns the user object specified by @p uuid.
    /// @see pubnub_get_uuidmetadata
    futres get_uuidmetadata(std::string const& uuid, std::string const& include)
    {
        return doit(pubnub_get_uuidmetadata(
                        d_pb,
                        include.c_str(),
                        uuid.c_str()));
    }

    /// Starts a transaction that deletes the user specified by @p uuid.
    /// @see pubnub_remove_uuidmetadata
    futres remove_uuidmetadata(std::string const& uuid)
    {
        return doit(pubnub_remove_uuidmetadata(d_pb, uuid.c_str()));
    }

    /// Starts a transaction that returns the spaces associated with the subscriber key.
    /// @see pubnub_getall_channelmetadata
    futres getall_channelmetadata(list_options& options)
    {
        return doit(pubnub_getall_channelmetadata(
                        d_pb, 
                        options.include(),
                        options.limit(),
                        options.start(),
                        options.end(),
                        options.count()));
    }

    /// Starts a transaction that creates a space with the attributes specified in @p channel_metadata_obj.
    /// @see pubnub_set_channelmetadata
    futres set_channelmetadata(std::string const& channel_metadataid, std::string const& include, std::string const& channel_metadata_obj)
    {
        return doit(pubnub_set_channelmetadata(
                        d_pb,
                        channel_metadataid.c_str(),
                        include.c_str(),
                        channel_metadata_obj.c_str()));
    }

    /// Starts a transaction that returns the space object specified by @p space_id.
    /// @see pubnub_get_channelmetadata
    futres get_channelmetadata(std::string const& channel_metadataid, std::string const& include)
    {
        return doit(pubnub_get_channelmetadata(
                        d_pb,
                        include.c_str(),
                        channel_metadataid.c_str()));
    }

    /// Starts a transaction that deletes the space specified with @p channel_metadataid.
    /// @see pubnub_remove_channelmetadata
    futres remove_channelmetadata(std::string const& channel_metadataid)
    {
        return doit(pubnub_remove_channelmetadata(d_pb, channel_metadataid.c_str()));
    }

    /// Starts a transaction that returns the space memberships of the user specified
    /// by @p user_id.
    /// @see pubnub_get_memberships
    futres get_memberships(std::string const& metadata_uuid, list_options& options)
    {
        return doit(pubnub_get_memberships(
                        d_pb,
                        metadata_uuid.c_str(),
                        options.include(),
                        options.limit(),
                        options.start(),
                        options.end(),
                        options.count()));
    }

    /// Starts a transaction that updates the space memberships of the user specified
    /// by @p metadata_uuid.
    /// @see pubnub_set_memberships
    futres set_memberships(std::string const& metadata_uuid,
                              std::string const& set_obj,
                              std::string const& include)
    {
        return doit(pubnub_set_memberships(
                        d_pb,
                        metadata_uuid.c_str(),
                        include.c_str(),
                        set_obj.c_str()));
    }

    /// Starts a transaction that removes the channel memberships of the user specified
    /// by @p metadata_uuid.
    /// @see pubnub_remove_memberships
    futres remove_memberships(std::string const& metadata_uuid,
                        std::string const& remove_obj,
                        std::string const& include)
    {
        return doit(pubnub_remove_memberships(
                        d_pb,
                        metadata_uuid.c_str(),
                        include.c_str(),
                        remove_obj.c_str()));
    }

    /// Starts a transaction that returns all users in the space specified by @p space_id.
    /// @see pubnub_get_members
    futres get_members(std::string const& channel_metadataid, list_options& options)
    {
        return doit(pubnub_get_members(
                        d_pb,
                        channel_metadataid.c_str(),
                        options.include(),
                        options.limit(),
                        options.start(),
                        options.end(),
                        options.count()));
    }

    /// Starts a transaction that adds the list of members of the space specified
    /// by @p channel_metadataid.
    /// @see pubnub_add_members
    futres add_members(std::string const& channel_metadataid,
                       std::string const& update_obj,
                       std::string const& include)
    {
        return doit(pubnub_add_members(
                        d_pb,
                        channel_metadataid.c_str(),
                        include.c_str(),
                        update_obj.c_str()));
    }

    /// Starts a transaction that updates the list of members of the space specified
    /// by @p channel_metadataid.
    /// @see pubnub_set_members
    futres set_members(std::string const& channel_metadataid,
                          std::string const& set_obj,
                          std::string const& include)
    {
        return doit(pubnub_set_members(
                        d_pb,
                        channel_metadataid.c_str(),
                        include.c_str(),
                        set_obj.c_str()));
    }

    /// Starts a transaction that removes the list of members of the space specified
    /// by @p channel_metadataid.
    /// @see pubnub_remove_members
    futres remove_members(std::string const& channel_metadataid,
                          std::string const& remove_obj,
                          std::string const& include)
    {
        return doit(pubnub_remove_members(
                        d_pb,
                        channel_metadataid.c_str(),
                        include.c_str(),
                        remove_obj.c_str()));
    }
#endif /* PUBNUB_USE_OBJECTS_API */

#if PUBNUB_USE_ACTIONS_API
    /// Starts a transaction that adds new type of message called action as a support for
    /// user reactions on a published messages.
    /// @see pubnub_add_message_action
    futres add_message_action(std::string const& channel,
                              std::string const& message_timetoken,
                              enum pubnub_action_type actype,
                              std::string const& value)
    {
        return doit(pubnub_add_message_action(
                        d_pb, 
                        channel.c_str(), 
                        message_timetoken.c_str(), 
                        actype, 
                        value.c_str()));
    }

    /// Returns message timetoken if previous transaction had been add_action()
    /// @see pubnub_get_message_timetoken()
    std::string get_message_timetoken()
    {
        pubnub_chamebl_t result = pubnub_get_message_timetoken(d_pb);
        return std::string(result.ptr, result.size);
    }

    /// Returns action timetoken if previous transaction had been add_action()
    /// @see pubnub_get_message_action_timetoken()
    std::string get_message_action_timetoken()
    {
        pubnub_chamebl_t result = pubnub_get_message_action_timetoken(d_pb);
        return std::string(result.ptr, result.size);
    }

    /// Initiates transaction that deletes(removes) previously added action on a
    /// published message.
    /// @see pubnub_remove_message_action()
    futres remove_message_action(std::string const& channel,
                                 std::string const& message_timetoken,
                                 std::string const& action_timetoken)
    {
        return doit(pubnub_remove_message_action(
                        d_pb,
                        channel.c_str(),
                        pubnub_str_2_chamebl_t((char*)message_timetoken.c_str()),
                        pubnub_str_2_chamebl_t((char*)action_timetoken.c_str())));
    }

    /// Initiates transaction that returns all actions added on a given @p channel
    /// between @p start and @p end action timetoken.
    /// @see pubnub_get_message_actions()
    futres get_message_actions(std::string const& channel,
                               std::string const& start,
                               std::string const& end,
                               size_t limit=0)
    {
        return doit(pubnub_get_message_actions(
                        d_pb,
                        channel.c_str(),
                        (start.size() > 0) ? start.c_str() : NULL,
                        (end.size() > 0) ? end.c_str() : NULL,
                        limit));
    }

    /// @see pubnub_get_message_actions_more()
    futres get_message_actions_more()
    {
        return doit(pubnub_get_message_actions_more(d_pb));
    }

    /// Initiates transaction that returns all actions added on a given @p channel
    /// between @p start and @p end message timetoken.
    /// @see pubnub_history_with_message_actions()
    futres history_with_message_actions(std::string const& channel,
                                        std::string const& start,
                                        std::string const& end,
                                        size_t limit=0)
    {
        return doit(pubnub_history_with_message_actions(
                        d_pb,
                        channel.c_str(),
                        (start.size() > 0) ? start.c_str() : NULL,
                        (end.size() > 0) ? end.c_str() : NULL,
                        limit));
    }

    /// @see pubnub_history_with_message_actions_more()
    futres history_with_message_actions_more()
    {
        return doit(pubnub_history_with_message_actions_more(d_pb));
    }
#endif /* PUBNUB_USE_ACTIONS_API */

#if PUBNUB_USE_AUTO_HEARTBEAT
    /// Enables keeping presence on subscribed channels and channel groups
    /// @see pubnub_enable_auto_heartbeat()
    int enable_auto_heartbeat(size_t period_sec)
    {
        return pubnub_enable_auto_heartbeat(d_pb, period_sec);
    }

    /// Sets(changes) heartbeat period for keeping presence on subscribed channels
    /// and channel groups.
    /// @see pubnub_set_heartbeat_period()
    int set_heartbeat_period(size_t period_sec)
    {
        return pubnub_set_heartbeat_period(d_pb, period_sec);
    }

    /// Disables keeping presence on subscribed channels and channel groups
    /// @see pubnub_disable_auto_heartbeat()
    void disable_auto_heartbeat()
    {
        pubnub_disable_auto_heartbeat(d_pb);
    }

    /// Returns whether(, or not) auto heartbeat on subscribed channels and channel
    /// groups is enabled
    /// @see pubnub_is_auto_heartbeat_enabled()
    bool is_auto_heartbeat_enabled()
    {
        return pubnub_is_auto_heartbeat_enabled(d_pb);
    }

    /// Enable "smart heartbeat" for presence management.
    /// @see pubnub_enable_smart_heartbeat()
    void enable_smart_heartbeat()
    {
        pubnub_enable_smart_heartbeat(d_pb);
    }

    /// Disable "smart heartbeat" for presence management.
    /// @see pubnub_disable_smart_heartbeat()
    void disable_smart_heartbeat()
    {
        pubnub_disable_smart_heartbeat(d_pb);
    }

    /// Releases all allocated heartbeat thumpers.
    /// Done on any object of the class, once, suffices.
    /// @see pubnub_heartbeat_free_thumpers()
    void heartbeat_free_thumpers()
    {
        pubnub_heartbeat_free_thumpers();
    }
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */
    
    /// Return the HTTP code (result) of the last transaction.
    /// @see pubnub_last_http_code
    int last_http_code() const
    {
        return pubnub_last_http_code(d_pb);
    }

    /// Return the string of the last publish transaction.
    /// @see pubnub_last_publish_result
    std::string last_publish_result() const
    {
        return pubnub_last_publish_result(d_pb);
    }

    /// Returns the string of the timetoken of the last `publish` transaction,
    /// as returned from Pubnub. If the last transaction is not a publish,
    /// or there is some other error, it returns "0".
    /// @see pubnub_last_publish_timetoken
    std::string last_publish_timetoken() const
    {
        return pubnub_last_publish_timetoken(d_pb);
    }

    /// Returns the result of parsing the last publish
    /// transaction's server response.
    /// @see pubnub_parse_publish_result
    pubnub_publish_res parse_last_publish_result()
    {
        return pubnub_parse_publish_result(last_publish_result().c_str());
    }

    /// Return the string of the last time token.
    /// @see pubnub_last_time_token
    std::string last_time_token() const
    {
        return pubnub_last_time_token(d_pb);
    }

    /// Sets whether to use (non-)blocking I/O according to option @p e.
    /// @see pubnub_set_blocking_io, pubnub_set_non_blocking_io
    int set_blocking_io(blocking_io e)
    {
        if (blocking == e) {
            return pubnub_set_blocking_io(d_pb);
        }
        else {
            return pubnub_set_non_blocking_io(d_pb);
        }
    }

    /// Sets the SSL/TLS options according to @p options
    /// @see pubnub_set_ssl_options
    void set_ssl_options(ssl_opt options)
    {
        pubnub_set_ssl_options(d_pb,
                               (options & useSSL) != 0,
                               (options & ignoreSecureConnectionRequirement) != 0);
    }

    /// Reuse SSL sessions, if possible (from now on).
    /// @see pubnub_set_reuse_ssl_session
    void reuse_ssl_session()
    {
        pubnub_set_reuse_ssl_session(d_pb, true);
    }

    /// Don't reuse SSL sessions (from now on).
    /// @see pubnub_set_reuse_ssl_session
    void dont_reuse_ssl_session()
    {
        pubnub_set_reuse_ssl_session(d_pb, false);
    }

    /// Use HTTP Keep-Alive on the context
    /// @see pubnub_use_http_keep_alive
    void use_http_keep_alive()
    {
        pubnub_use_http_keep_alive(d_pb);
    }

    /// Don't Use HTTP Keep-Alive on the context
    /// @see pubnub_dont_use_http_keep_alive
    void dont_use_http_keep_alive()
    {
        pubnub_dont_use_http_keep_alive(d_pb);
    }

#if PUBNUB_PROXY_API
    /// Manually set a proxy to use
    /// @see pubnub_set_proxy_manual
    int set_proxy_manual(proxy_type         protocol,
                         std::string const& ip_address_or_url,
                         uint16_t           port)
    {
        pubnub_proxy_type ccore_proxy = ccore_proxy_type(protocol);
        return pubnub_set_proxy_manual(
            d_pb, ccore_proxy, ip_address_or_url.c_str(), port);
    }

    /// Sets a proxy to none
    /// @see pubnub_set_proxy_none
    void set_proxy_none()
    {
        pubnub_set_proxy_none(d_pb);
    }

    /// Set the proxy to use from system configuration.
    /// @see pubnub_set_proxy_from_system
    int set_proxy_from_system(proxy_type protocol)
    {
        pubnub_proxy_type ccore_proxy = ccore_proxy_type(protocol);
        return pubnub_set_proxy_from_system(d_pb, ccore_proxy);
    }

    /// Sets the authentication scheme to use for proxy
    /// @see pubnub_set_proxy_authentication
    int set_proxy_authentication_username_password(std::string const& username,
                                                   std::string const& password)
    {
        return pubnub_set_proxy_authentication_username_password(
            d_pb, username.c_str(), password.c_str());
    }

    /// Sets to use no authentication scheme for proxy
    /// @see pubnub_set_proxy_authentication_scheme_none
    int set_proxy_authentication_none()
    {
        return pubnub_set_proxy_authentication_none(d_pb);
    }
#endif

#if __cplusplus >= 201103L
    /// Sets the transaction timeout duration
    int set_transaction_timeout(std::chrono::milliseconds duration)
    {
        return pubnub_set_transaction_timeout(d_pb, duration.count());
    }
    /// Returns the transaction timeout duration
    std::chrono::milliseconds transaction_timeout_get()
    {
        std::chrono::milliseconds result(pubnub_transaction_timeout_get(d_pb));
        return result;
    }
    /// Sets the connection timeout duration
    int set_connection_timeout(std::chrono::milliseconds duration)
    {
        return pubnub_set_wait_connect_timeout(d_pb, duration.count());
    }
    /// Returns the connection timeout duration
    std::chrono::milliseconds connection_timeout_get()
    {
        std::chrono::milliseconds result(pubnub_wait_connect_timeout_get(d_pb));
        return result;
    }
#else
    /// Sets the transaction timeout duration, in milliseconds
    int set_transaction_timeout(int duration_ms)
    {
        return pubnub_set_transaction_timeout(d_pb, duration_ms);
    }
    /// Returns the transaction timeout duration, in milliseconds
    int transaction_timeout_get()
    {
        return pubnub_transaction_timeout_get(d_pb);
    }
    /// Sets the connection timeout duration, in milliseconds
    int set_connection_timeout(int duration_ms)
    {
        return pubnub_set_wait_connect_timeout(d_pb, duration_ms);
    }
    /// Returns the connection timeout duration, in milliseconds
    int connection_timeout_get()
    {
        return pubnub_wait_connect_timeout_get(d_pb);
    }
#endif

    /// Frees the context and any other thing that needs to be
    /// freed/released.
    /// @see pubnub_free
    int free()
    {
        int rslt = pubnub_free(d_pb);
        if (0 == rslt) {
            d_pb = 0;
        }
        return rslt;
    }

#if __cplusplus >= 201103L
    /// Frees the context and any other thing that needs to be
    /// freed/released - with timeout @p duration
    /// @see pubnub_free_with_timeout()
    int free_with_timeout(std::chrono::milliseconds duration)
    {
        int rslt = pubnub_free_with_timeout(d_pb, duration.count());
        if (0 == rslt) {
            d_pb = 0;
        }
        return rslt;
    }
#else
    /// Frees the context and any other thing that needs to be
    /// freed/released - with timeout @p duration_ms milliseconds
    /// @see pubnub_free_with_timeout()
    int free_with_timeout(int duration_ms)
    {
        int rslt = pubnub_free_with_timeout(d_pb, duration_ms);
        if (0 == rslt) {
            d_pb = 0;
        }
        return rslt;
    }
#endif

    /// Enables safe exit from the main() in callback environment by disabling
    /// platform watcher thread.
    /// @see pubnub_stop()
    void stop(void)
    {
#if defined(PUBNUB_CALLBACK_API)
        pubnub_stop();
#endif
    }

#if PUBNUB_CRYPTO_API
    /// Set the crypto module to be set by the context
    ///
    /// This function sets the crypto module to be used by the context
    /// for encryption and decryption of messages.
    /// @see pubnub_set_crypto_module()
    void set_crypto_module(into_crypto_provider_ptr &crypto)
    {
        pubnub_set_crypto_module(d_pb, crypto.into_provider());
    }

    /// Get the crypto module used by the context
    ///
    /// This function gets the crypto module used by the context
    /// for encryption and decryption of messages.
    /// @see pubnub_get_crypto_module()
    pubnub_crypto_provider_t *get_crypto_module()
    {
        return pubnub_get_crypto_module(d_pb);
    }
#endif /* PUBNUB_CRYPTO_API */

    ~context()
    {
        if (d_pb) {
            pubnub_free_with_timeout(d_pb, 1000);
        }
        pubnub_mutex_destroy(d_mutex);
    }

private:
    // pubnub context is not copyable
    context(context const&);

    // internal helper function
    futres doit(pubnub_res e) { return futres(d_pb, *this, e); }
    /// The mutex protecting d_auth field
    mutable pubnub_mutex_t d_mutex;
    /// The publish key
    std::string d_pubk;
    /// The subscribe key
    std::string d_ksub;
    /// The secret key
    std::string d_secret_key;
    /// The auth key
    std::string d_auth;
    /// The auth token containing pam permissions
    std::string d_auth_token;
    /// The origin set last time (doen't have to be the one used,
    /// the default can be used instead)
    std::string d_origin;
    /// The (C) Pubnub context
    pubnub_t* d_pb;
};

} // namespace pubnub

#endif // !defined INC_PUBNUB_COMMON_HPP
