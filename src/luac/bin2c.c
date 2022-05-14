#include <stdlib.h>
#include <stdio.h>
#include <string.h>

unsigned char buf[65536];

int main(int argc, char *argv[])
{
  int i, k, n;
  char line[256], aux[8];

  if (argc != 2)
    exit(1);

  if ((n = read(0, buf, sizeof(buf))) <= 0)
    exit(1);

  printf("static unsigned char %s[%d] = {\n", argv[1], n);
  line[0] = 0;
  k = 0;

  for (i = 0; i < n; i++) {
    sprintf(aux, "0x%02x, ", buf[i]);
    strcat(line, aux);
    k++;
    if (k == 8) {
      printf("  %s\n", line);
      line[0] = 0;
      k = 0;
    }
  }
  if (k)
    printf("  %s\n", line);
  printf("};\n");

  exit(0);
}
