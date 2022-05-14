#include "p.h"
#include "check.h"

Err checkResource(UInt32 type, UInt16 id, UInt8 *good_sum)
{
  MemHandle h;
  UInt8 *p, test_sum[CHECKLEN];
  UInt16 n, i;

  if ((h = DmGet1Resource(type, id)) == NULL)
    return -1;

  if ((p = MemHandleLock(h)) == NULL) {
    DmReleaseResource(h);
    return -2;
  }

  n = MemHandleSize(h);
  EncDigestMD5(p, n, test_sum);

  MemHandleUnlock(h);
  DmReleaseResource(h);

  for (i = 0; i < CHECKLEN; i++) {
    if (test_sum[i] != good_sum[i])
      return -3;
  }

  return 0;
}
