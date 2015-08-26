#if !defined INC_PNC_SUBSCRIPTIONS
#define      INC_PNC_SUBSCRIPTIONS


void pnc_subscribe_list_channels(char *value, unsigned max_length);

void pnc_subscribe_list_channel_groups(char *value, unsigned max_length);

void pnc_subscribe_add_channel(char const *channel);

void pnc_subscribe_add_channel_group(char const *channel);

void pnc_subscribe_remove_channel(char const *channel);

void pnc_subscribe_remove_channel_group(char const *channel_group);


#endif /* !defined INC_PNC_SUBSCRIPTIONS */
