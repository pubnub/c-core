/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_AUTO_HEARTBEAT

#if !defined INC_PBSTR_REMOVE_FROM_LIST
#define INC_PBSTR_REMOVE_FROM_LIST

/** Removes members of comma separated @p leave_list from comma separated @p list.
  */
void pbstr_remove_from_list(char* list, const char* leave_list);

/** Called only if list is allocated dinamically.
    Frees memory if list string is empty.
  */
void pbstr_free_if_empty(char** list);

#endif /* !defined INC_PBSTR_REMOVE_FROM_LIST */

#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

