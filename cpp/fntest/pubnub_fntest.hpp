/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST
#define      INC_PUBNUB_FNTEST

#include "pubnub.hpp"

extern "C" {
#include "core/pubnub_helper.h"
}

#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <sstream>
#include <iostream>


namespace pubnub {

    /// Helper to put a `pubnub_res` to a standard stream
/*    inline std::ostream& operator<<(std::ostream&out, pubnub_res e) {
        return out << (int)e << '(' << pubnub_res_2_string(e) << ')';
        }*/


    /// A message recevied on a channel
    class msg_on_chan {
        std::string d_message;
        std::string d_channel;
    public:
        msg_on_chan() {}
        msg_on_chan(std::string message, std::string channel) :
            d_message(message), d_channel(channel) {
        }

        std::string const &message() const { return d_message; }
        std::string const &channel() const { return d_channel; }

        bool operator==(msg_on_chan const &x) const {
            return (d_message == x.d_message) && (d_channel == x.d_channel);
        }
        bool operator<(msg_on_chan const &x) const {
            return (d_message == x.d_message) ? (d_channel < x.d_channel) : (d_message < x.d_message);
        }
    };
    
    /// Returns whether the given messages have been received on the context
    inline bool got_messages(context const&pb, std::vector<std::string> const &msg) {
        auto msg_sorted(msg);
        auto all_sorted(pb.get_all());
        if(1 < all_sorted.size()) {
            std::sort(msg_sorted.begin(),msg_sorted.end());
            std::sort(all_sorted.begin(),all_sorted.end());
        }
        return all_sorted == msg_sorted;
    }
    

    /// Returns whether the given message was received on the given
    /// channel on the context
    inline bool got_message_on_channel(context const &pb, msg_on_chan expected) {
        pubnub::msg_on_chan got(pb.get(), pb.get_channel());
        return got == expected;
    }
    
    
    /// A helper to do a set difference on vectors
    template<typename T>
    std::vector<T>& inplace_set_difference(std::vector<T> &from, std::vector<T> &to_remove) {
        auto it = std::set_difference(from.begin(), from.end(),
                                      to_remove.begin(), to_remove.end(),
                                      from.begin());
        from.resize(it - from.begin());
        return from;
    }
    

    /// Get all messages from context and put them to a vector of
    /// "message on channel" objects
    std::vector<msg_on_chan> get_all_msg_on_chan(context const&pb);


    /// Wait for the @p futr to finish a Pubnub transaction in @p
    /// rel_time and give the result (if it finishes) to @p pbresult.
    /// On output, @p rel_time will have the time remaining until
    /// the timeout (as specificied by the input value of @p rel_time).
    /// @return true: transaction completed in time, false: timeout
    bool wait_for(futres &futr, std::chrono::milliseconds &rel_time, pubnub_res &pbresult);

    /// Wait for both the @p futr and @p futr_2 to finish a Pubnub
    /// transaction (each) in @p rel_time and give the result (if it
    /// finishes) to @p pbresult.  On output, @p rel_time will have
    /// the time remaining until the timeout (as specificied by the
    /// input value of @p rel_time).
    ///
    /// @return true: transaction completed in time with same result,
    /// false: timeout or different results
    bool wait_for(futres &futr_1,
                  futres &futr_2,
                  std::chrono::milliseconds &rel_time,
                  pubnub_res &pbresult_1,
                  pubnub_res &pbresult_2);

    /// Do a subscribe operation on context @p pb and channel @p channel
    /// and channel group @p chgroup, wait for @p rel_time, and
    /// check that all the messages (on channels) are received as
    /// specified in @p expected. Other messages that are received but
    /// are not in @p expected are ignored.
    /// @return true: subscribe completed and all expected messages
    /// received, false: otherwise
    bool subscribe_and_check(context &pb, std::string const&channel, const std::string &chgroup, std::chrono::milliseconds rel_time, std::vector<msg_on_chan> expected);

    /** Helper template class to handle "expectations" in tests.
        Usually used with the #EXPECT macro.
     */
    template <typename T>
    class expect {
        T d_t;
        char const *d_expr;
        char const *d_fname;
        long d_line;
        bool d_checked;
        char const *d_descr;
        
        std::string make_description(char const *op, T expected) {
            std::stringstream stream;
            stream << std::boolalpha << 
                "For expression '"  << d_expr << "'" << 
                std::endl << "  in file: " << d_fname << " line: " <<  d_line;
            if (d_descr) {
                stream << std::endl << "  description: " << d_descr;
            }
            stream << std::endl << "  with check: '" << op <<
                "', got: " << d_t << ", expected: " << expected;
            return stream.str();
        }
    public:
        expect(T t, char const *expr, char const *fname, long line, char const *descr=NULL) 
            : d_t(t)
            , d_expr(expr)
            , d_fname(fname)
            , d_line(line)
            , d_checked(false)
            , d_descr(descr)
            {}
        ~expect() 
#if __cplusplus >= 201103L
            noexcept(false)
#endif
        {
            if (!d_checked) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description("left unchecked!", d_t));
            }
        }
        void operator==(T t) {
            d_checked = true;
            if (d_t != t) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description("==", t));
            }
        }
        void operator!=(T t) {
            d_checked = true;
            if (d_t == t) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description("!=", t));
            }
        }
        void operator<(T t) {
            d_checked = true;
            if (!(d_t < t)) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description("<", t));
            }
        }
        void operator<=(T t) {
            d_checked = true;
            if (!(d_t <= t)) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description("<=", t));
            }
        }
        void operator>(T t) {
            d_checked = true;
            if (!(d_t > t)) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description(">", t));
            }
        }
        void operator>=(T t) {
            d_checked = true;
            if (!(d_t >= t)) {
                std::cout << __LINE__ << std::endl;
                throw std::runtime_error(make_description(">=", t));
            }
        }
    };

#define TEST_DECL(tst)                                                  \
    void pnfn_test_##tst(std::string const &pubkey,                     \
                         std::string const &keysub,                     \
                         std::string const &origin,                     \
                         bool const &cannot_do)                        

#define TEST_DEF(tst)                                                   \
    TEST_DECL(tst)                                                      \
    {                                                                   \
        char const* const this_test_name_ = #tst;                       \
        /* Eliminating unused variable warning */                       \
        do {                                                            \
        } while (sizeof cannot_do == 0);

#define TEST_ENDDEF }

    /** Simple class that gives description for indeterminate test */
    class except_test {
        std::string d_str;
        char const *d_file;
        long d_line;
        char const *d_expr;
    public:
        except_test(std::string &str, char const *f, long l, char const *e)
            : d_str(str)
            , d_file(f)
            , d_line(l)
            , d_expr(e)
            {}      
        std::string what() const {
            std::stringstream stream;
            stream << "In file:" << d_file << ", line:" << d_line << " " <<
                ((0 != *d_expr)?"For expression:'":"") << d_expr << "' - got:'" << d_str << "'";
            return stream.str();
        }
    };

    inline except_test make_test_exception(std::string describe,
                                           char const *f,
                                           long l,
                                           char const *expr)
    { 
        return except_test(describe, f, l, expr);
    }

#define test_exception(str, fname, line, expr) make_test_exception((str), (fname), (line), (expr))

/** Declare a test that needs support for channel groups API (add,
    remove, list: channel groups). IOW, can only be run on an account
    that has this API enabled.
*/
#define TEST_DEF_NEED_CHGROUP(tst)                                                 \
    TEST_DEF(tst)                                                                  \
    {                                                                              \
        if (cannot_do) {                                                           \
            throw test_exception("Test needs channel groups.", __FILE__, __LINE__, "");\
        }                                                                          \
    }

#define pbpub_outof_quota(ft, rslt)                                                \
    (((rslt) == PNR_PUBLISH_FAILED)                                                \
     && (PNPUB_ACCOUNT_QUOTA_EXCEEDED == (ft).parse_last_publish_result()))

#define check_result_for_exceptions(ft, rslt, fname, line, expr)                   \
    if ((rslt) == PNR_ABORTED) {                                                   \
        throw test_exception("Transaction aborted.", (fname), (line), (expr));     \
    }                                                                              \
    else if (pbpub_outof_quota((ft), (rslt))) {                                    \
        throw test_exception("Out of quota.", (fname), (line), (expr));            \
    }                                                                       

    /** Helper class to set up a check for a Pubnub transaction.
        Usually to be used with the #SENSE macro.
     */
    class sense {
        futres d_ft;
        pubnub_res d_result;
        char const *d_expr;
        char const *d_fname;
        long d_line;
    public:
        sense(futres ft, char const *expr, char const *fname, long line) 
            : d_ft(std::move(ft))
            , d_result(PNR_TIMEOUT) 
            , d_expr(expr)
            , d_fname(fname)
            , d_line(line)
            {}

        sense &in(std::chrono::milliseconds deadline) {
            bool trans_finished(wait_for(d_ft, deadline, d_result));
            if(trans_finished) {                
                check_result_for_exceptions(d_ft, d_result, d_fname, d_line, d_expr);
            }
            expect<bool>(trans_finished, d_expr, d_fname, d_line, "transaction timed out") == true;
            return *this;
        }
        sense &before(std::chrono::milliseconds deadline) { return in(deadline); }
        sense &operator<<(std::chrono::milliseconds deadline) { return in(deadline); }
        
        void operator==(pubnub_res expected) {
            expect<pubnub_res>(d_result, d_expr, d_fname, d_line) == expected;
        }
        futres &get_futres() { return d_ft; }
        char const *expr() const { return d_expr; }
        char const *fname() const { return d_fname; }
        long line() const { return d_line; }
    };
    
    /** This expects the outcome of two Pubnub transactions in parallel.
        It expects both of them to finish with the same outcome.
        In general, not to be used directly, but via operator `&` of
        #sense objects.
     */
    class sense_double {
        futres &d_left;
        futres &d_right;
        pubnub_res d_result_left;
        pubnub_res d_result_right;
        char const *d_expr_left;
        char const *d_expr_right;
        char const *d_fname;
        long d_line;
    public:
        sense_double(sense& left, sense& right)
            : d_left(left.get_futres())
            , d_right(right.get_futres()) 
            , d_expr_left(left.expr())
            , d_expr_right(right.expr())
            , d_fname(left.fname())
            , d_line(left.line())
        {
        }
        sense_double& in(std::chrono::milliseconds deadline)
        {
            std::string expr_left(d_expr_left);
            std::string expr_right(d_expr_right);
            bool trans_finished(wait_for(d_left, d_right, deadline, d_result_left, d_result_right));
            if(trans_finished) {                
                check_result_for_exceptions(d_left, d_result_left, d_fname, d_line, d_expr_left);
                check_result_for_exceptions(d_right, d_result_right, d_fname, d_line, d_expr_right);
            }
            expect<bool>(trans_finished,
                         (expr_left + " && " + expr_right).c_str(),
                         d_fname,
                         d_line,
                         "waiting for two contexts timed out") == true;
            return *this;
        }
        sense_double &before(std::chrono::milliseconds deadline) { return in(deadline); }
        sense_double &operator<<(std::chrono::milliseconds deadline) { return in(deadline); }
        
        void operator==(pubnub_res expected) {
            std::string expr_left(d_expr_left);
            std::string expr_right(d_expr_right);
            if(d_result_left == d_result_right) {
                expect<pubnub_res>(d_result_right,
                                   (expr_left + " && " + expr_right).c_str(),
                                   d_fname,
                                   d_line) == expected;
            }
            else {
                expect<bool>(false,
                             (expr_left + " && " + expr_right).c_str(),
                             d_fname,
                             d_line,
                             "waiting for two contexts failed") == true;
            }
        }
    };
    

    /// Returns a combined checker for two transactions at the same time
    inline sense_double operator&&(sense &left, sense &right) {
        return sense_double(left, right);
    }

#if defined(__GNUC__)
    /// Returns a combined checker for two transactions at the same time - non reference operands
    inline sense_double operator&&(sense left, sense right) {
        return sense_double(left, right);
    }
#endif
 /** Helper macro to capture source code info at the place of
     check in the test.
 */    
#define SENSE(expr) sense(expr, #expr, __FILE__, __LINE__)

    template <typename T> expect<T> make_expect(T t, char const *e, char const *f, long l) { return expect<T>(t, e, f, l); }
    
/** Helper macro to capture source code info at the place of
    check in the test.
 */    
#define EXPECT(expr) make_expect(expr, #expr, __FILE__, __LINE__)

#define EXPECT_RESULT(ft, rslt)                                      \
    check_result_for_exceptions(ft, rslt, __FILE__, __LINE__, #rslt);\
    EXPECT(rslt)

/** Someone might find this easier to read, especially on longer checks.
 */    
#define EXPECT_TRUE(expr) EXPECT(expr) == true

/** See
    https://support.pubnub.com/support/solutions/articles/14000043769-what-are-valid-channel-names-
*/
#define MAX_PUBNUB_CHAN_NAME 92

    inline std::string pnfntst_make_name(char const* s) {
        std::string grn(std::to_string(rand()));
        std::string str(s);
        str += "_" + grn;
        if(MAX_PUBNUB_CHAN_NAME < str.size()) {
            str.resize(MAX_PUBNUB_CHAN_NAME);
        }
        return str;
    }

    inline void await_console() {
        char s[1024];
        std::cin.getline(s, sizeof s);
    }
    
}


#endif  // !defined INC_PUBNUB_FNTEST
