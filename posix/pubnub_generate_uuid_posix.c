#include "core/pubnub_generate_uuid.h"

#include <fcntl.h>
#include <unistd.h>




int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
  ssize_t n;
  int fd = open("/dev/urandom", O_RDONLY);

  if (fd < 0) {
    return -1;
  }

  n = read(fd, uuid, sizeof *uuid);
  close(fd);

  if (n < 0) {
    return -1;
  }

  uuid->uuid[6] &= 0x0F;
  uuid->uuid[6] |= 0x40;
  uuid->uuid[8] &= 0x3F;
  uuid->uuid[8] |= 0x80;

  return 0;
}
