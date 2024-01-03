/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ACTIONS_API

#if !defined INC_PUBNUB_ACTIONS_API
#define INC_PUBNUB_ACTIONS_API

#include "lib/pb_extern.h"
#include "pbcc_actions_api.h"

#include <stdbool.h>


/** Adds new type of message called action as a support for user reactions on a published
    messages.
    Json string @p value is checked for its quotation marks at its ends. If any of the
    quotation marks is missing function returns parameter error.
    If the transaction is finished successfully response will have 'data' field with
    added action data. If there is no data, nor error description in the response,
    response parsing function returns format error.
    @param pb The pubnub context. Can't be NULL
    @param channel The channel on which action is referring to.
    @param message_timetoken The timetoken(unquoted) of a published message action is
                             applying to
    @param actype Action type
    @param value Json string describing the action that is to be added
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_add_message_action(pubnub_t* pb,
                                          char const* channel,
                                          char const* message_timetoken,
                                          enum pubnub_action_type actype,
                                          char const* value);


/** Searches the response(if previous transaction on the @p pb context had been
    pubnub_add_message_action and was accomplished successfully) and retrieves timetoken of
    a message action was added on.
    If key expected is not found, preconditions(about right transaction) were not fulfilled,
    or error was encountered,
    returned structure has 0 'size' field and NULL 'ptr' field.
    @param pb The pubnub context. Can't be NULL
    @return Structured pointer to memory block containing message timetoken value(including
            its quotation marks) within the context response buffer
  */
PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_message_timetoken(pubnub_t* pb);


/** Searches the response(if previous transaction on the @p pb context had been
    pubnub_add_message_action and was accomplished successfully) and retrieves timetoken of a
    resently added action.
    If key expected is not found, preconditions were not fulfilled, or error was encountered,
    returned structure has 0 'size' field and NULL 'ptr' field.
    @param pb The pubnub context. Can't be NULL
    @return Structured pointer to memory block containing action timetoken value(including
            its quotation marks) within the context response buffer
  */
PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_message_action_timetoken(pubnub_t* pb);


/** Initiates transaction that deletes(removes) previously added action on a published message.
    If there is no success confirming data, nor error description in the response it is
    considered format error.
    @param pb The pubnub context. Can't be NULL
    @param channel The channel on which action was previously added.
    @param message_timetoken The timetoken of a published message action was applied to.
                             (Obtained from the response when action was added and expected
                              with its quotation marks at both ends)
    @param action_timetoken The action timetoken when it was added(Gotten from the transaction
                            response when action was added and expected with its quotation
                            marks at both ends)
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_message_action(pubnub_t* pb,
                                             char const* channel,
                                             pubnub_chamebl_t message_timetoken,
                                             pubnub_chamebl_t action_timetoken);


/** Initiates transaction that returns all actions added on a given @p channel between @p start
    and @p end action timetoken.
    The response to this transaction can be partial and than it contains the hyperlink string
    value to the rest.
    @see pubnub_get_message_actions_more()
    If there is no actions data, nor error description in the response it is considered
    format error.
    @param pb The pubnub context. Can't be NULL
    @param channel The channel on which actions are observed.
    @param start Start action timetoken(unquoted). Can be NULL meaning there is no lower
                 limitation in time.
    @param end End action timetoken(unquoted). Can be NULL in which case upper time limit is
               present moment.
    @param limit Number of actions to return in response. Regular values 1 - 100. If you set `0`,
                 that means “use the default”. At the time of this writing, default was 100.
                 Any value greater than 100 is considered an error.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_message_actions(pubnub_t* pb,
                                           char const* channel,
                                           char const* start,
                                           char const* end,
                                           size_t limit);


/** This function expects previous transaction to be the one for reading the actions and
    that it was successfully accomplished. If it is not the case, returns corresponding
    error.
    When preconditions are fulfilled, it searches the hyperlink to the rest in the existing
    response context buffer which it uses for obtaining another part of the server response.
    Anotherwords, once the hyperlink is found in the existing response it is used for
    initiating new request and function than behaves, essentially, as pubnub_get_actions().
    If there is no hyperlink encountered in the previous transaction response it
    returns success: PNR_GOT_ALL_ACTIONS meaning that the answer is complete.
    @param pb The pubnub context containing response buffer to be searched. Can't be NULL
    @retval PNR_STARTED transaction successfully initiated.
    @retval PNR_GOT_ALL_ACTIONS transaction successfully finished.
    @retval corresponding error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_message_actions_more(pubnub_t* pb);


/** Initiates transaction that returns all actions added on a given @p channel between @p start
    and @p end message timetoken.
    The response to this transaction can be partial and than it contains the hyperlink string
    value to the rest.
    @see pubnub_history_with_message_actions_more()
    If there is no actions data, nor error description in the response it is considered
    format error.
    @param pb The pubnub context. Can't be NULL
    @param channel The channel on which actions are observed.
    @param start Start message timetoken(unquoted). Can be NULL meaning there is no lower
                 limitation in time.
    @param end End message timetoken(unquoted). Can be NULL in which case upper time limit
               is present moment.
    @param limit Number of messages to return in response. Regular values 1 - 100. If you
                 set `0`, that means “use the default”. At the time of this writing, default
                 was 100. Any value greater than 100 is considered an error.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_history_with_message_actions(pubnub_t* pb,
                                                    char const* channel,
                                                    char const* start,
                                                    char const* end,
                                                    size_t limit);


/** This function expects previous transaction to be the one for reading the history with
    actions and that it was successfully accomplished. If it is not the case, returns
    corresponding error.
    When preconditions are fulfilled it searches for the hyperlink to the rest of the answer
    in the existing response context buffer which it uses for obtaining another
    part of the server response. Anotherwords, once the hyperlink is found in the existing
    response it is used for initiating new request and function than behaves as
    pubnub_history_with_message_actions().
    If there is no hyperlink encountered in the previous transaction response it
    returns success: PNR_GOT_ALL_ACTIONS meaning that the answer is complete.
    @param pb The pubnub context containing response buffer to be searched. Can't be NULL
    @retval PNR_STARTED transaction successfully initiated.
    @retval PNR_GOT_ALL_ACTIONS transaction successfully finished.
    @retval corresponding error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_history_with_message_actions_more(pubnub_t* pb);


#endif /* !defined INC_PUBNUB_ACTIONS_API */

#endif /* PUBNUB_USE_ACTIONS_API */

