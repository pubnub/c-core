/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_TRIBOOL_HPP
#define INC_TRIBOOL_HPP

namespace pubnub {
    
/** A simple C++ tribool type. It has `true`, `false` and
    `not_set`.
 */
class tribool {
public:
    enum not_set_t { not_set = 2 };
    tribool()
        : d_3log(not_set)
    {
    }
    tribool(enum pubnub_tribool v)
        : d_3log(v)
    {
    }
    tribool(bool v)
        : d_3log(v)
    {
    }
    tribool(not_set_t)
        : d_3log(not_set)
    {
    }

    tribool operator!() const
    {
        static const tribool rslt[3] = { true, false, not_set };
        return rslt[d_3log];
    }
    bool is_set() const { return d_3log != not_set; }

    tribool operator&&(bool t) const { return *this && tribool(t); }
    tribool operator&&(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, false, false },
                                            { false, true, not_set },
                                            { false, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }

    tribool operator||(bool t) const { return *this || tribool(t); }
    tribool operator||(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, true, not_set },
                                            { true, true, true },
                                            { not_set, true, not_set } };
        return rslt[d_3log][t.d_3log];
    }

    tribool operator==(tribool t) const
    {
        static const tribool rslt[3][3] = { { true, false, not_set },
                                            { false, true, not_set },
                                            { not_set, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }
    tribool operator==(bool t) const
    {
        return !is_set() ? not_set
                         : static_cast<tribool>(static_cast<bool>(d_3log) == t);
    }

    tribool operator!=(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, true, not_set },
                                            { true, false, not_set },
                                            { not_set, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }
    tribool operator!=(bool t) const
    {
        return !is_set() ? not_set
                         : static_cast<tribool>(static_cast<bool>(d_3log) != t);
    }

    operator bool() const { return true == static_cast<bool>(d_3log); }

    std::string to_string() const
    {
        static char const* rslt[3] = { "false", "true", "not-set" };
        return rslt[d_3log];
    }

private:
    int d_3log;
};

inline tribool operator==(tribool, tribool::not_set_t)
{
    return tribool::not_set;
}

inline tribool operator!=(tribool, tribool::not_set_t)
{
    return tribool::not_set;
}

} // namespace pubnub

#endif // !defined INC_TRIBOOL_HPP
