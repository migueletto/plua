#include <Standalone.h>
#include <stdint.h>
#include <stdlib.h>

STANDALONE_CODE_RESOURCE_ID(1000);

void main(void);

unsigned long PealArmStart(void *arg)
{
  main();
  return 0;
}
