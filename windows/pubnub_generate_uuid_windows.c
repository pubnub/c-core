#include "core/pubnub_generate_uuid.h"

#include <rpc.h>


int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
  UUID win_uuid;
  if (RPC_S_OK != UuidCreate(&win_uuid)) {
    return -1;
  }
  memcpy(uuid, &win_uuid, sizeof *uuid);

  return 0;
}
