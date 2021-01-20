#include "core/pubnub_generate_uuid.h"

#if __UWP__
#include <combaseapi.h>
#else
#include <rpc.h>
#endif

int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
  UUID win_uuid;
#if __UWP__
  if (S_OK != CoCreateGuid(&win_uuid)) {
      return -1;
  }
#else
  if (RPC_S_OK != UuidCreate(&win_uuid)) {
    return -1;
  }
#endif
  memcpy(uuid, &win_uuid, sizeof *uuid);

  return 0;
}
