#!/bin/sh

awk '
BEGIN {
  print "HEX #hlpI# ID 1001";
}
{
  b1 = $1 / 256;
  b2 = $1 % 256;
  printf("  0x%02X 0x%02X\n", b1, b2);
  b1 = $2 / 256;
  b2 = $2 % 256;
  printf("  0x%02X 0x%02X\n", b1, b2);
}
END {
  print "DATA #hlpW# ID 1001 #words.dat#";
  print "DATA #hlpD# ID 1001 #defs.dat#";
} ' | tr '#' '"'

exit 0
