#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  unsigned int i1, i2;
  unsigned short s[2];

  while (!feof(stdin)) {
    fscanf(stdin, "%d\t%d\n", &i1, &i2);
    s[0] = (i1 >> 8) | (i1 << 8);
    s[1] = (i2 >> 8) | (i2 << 8);
    write(1, s, 2*sizeof(short));
  }
  
  exit(0);
}
