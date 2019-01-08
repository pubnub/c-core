/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_COSOLE_TEXT_PAINT
#define INC_COSOLE_TEXT_PAINT

#include "core/pubnub_assert.h"

#include <windows.h>

#define FOREGROUND_YELLOW 0x6
#define FOREGROUND_WHITE 0x7
#define FOREGROUND_WHITE_WITH_BACKGROUND_RED 0xcf

#define paint_text_white()                                                     \
    do {                                                                       \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                          \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                  \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                              \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                 \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {              \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                         \
        }                                                                      \
        SetConsoleTextAttribute(m_hstdout_,                                    \
                                FOREGROUND_WHITE | FOREGROUND_INTENSITY);      \
    } while (0)

#define paint_text_green()                                                     \
    do {                                                                       \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                          \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                  \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                              \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                 \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {              \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                         \
        }                                                                      \
        SetConsoleTextAttribute(m_hstdout_,                                    \
                                FOREGROUND_GREEN | FOREGROUND_INTENSITY);      \
    } while (0)

#define paint_text_blue()                                                      \
    do {                                                                       \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                          \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                  \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                              \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                 \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {              \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                         \
        }                                                                      \
        SetConsoleTextAttribute(m_hstdout_,                                    \
                                FOREGROUND_BLUE | FOREGROUND_INTENSITY);       \
    } while (0)

#define paint_text_red()                                                            \
    do {                                                                            \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                               \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                       \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                                   \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                      \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {                   \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                              \
        }                                                                           \
        SetConsoleTextAttribute(m_hstdout_, FOREGROUND_RED | FOREGROUND_INTENSITY); \
    } while (0)

#define paint_text_yellow()                                                    \
    do {                                                                       \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                          \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                  \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                              \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                 \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {              \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                         \
        }                                                                      \
        SetConsoleTextAttribute(m_hstdout_,                                    \
                                FOREGROUND_YELLOW | FOREGROUND_INTENSITY);     \
    } while (0)

#define paint_text_white_with_background_red()                                                    \
    do {                                                                       \
        m_hstdout_ = GetStdHandle(STD_OUTPUT_HANDLE);                          \
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo_;                                  \
        m_wOldColorAttrs_ = FOREGROUND_INTENSITY;                              \
        PUBNUB_ASSERT_OPT(m_hstdout_ != INVALID_HANDLE_VALUE);                 \
        if (GetConsoleScreenBufferInfo(m_hstdout_, &csbiInfo_)) {              \
            m_wOldColorAttrs_ = csbiInfo_.wAttributes;                         \
        }                                                                      \
        SetConsoleTextAttribute(m_hstdout_, FOREGROUND_WHITE_WITH_BACKGROUND_RED);\
    } while (0)

#define reset_text_paint()                                                     \
    SetConsoleTextAttribute(m_hstdout_, m_wOldColorAttrs_)

static HANDLE m_hstdout_;
static WORD   m_wOldColorAttrs_;

#endif /* !defined INC_COSOLE_TEXT_PAINT */
