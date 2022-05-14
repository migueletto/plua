#!/bin/sh

if [ $# -ne 1 ]; then
  exit 1
fi

rm -f words.txt defs.txt

sort $1 | awk '
BEGIN {
  pw = 0;
  pd = 0;
  print "HEX #hlpI# ID 1001";
}
/^[^#]/ {
  i = index($0,": ");
  word = substr($0,1,i-1);
  def = $0;

  i1 = index(word,"(");
  i2 = index(word,")");
  if (i1 > 0 && i2 == length(word))
    word = substr(word,1,i1-1);

  print word >> "words.txt";
  print def >> "defs.txt";

  b1 = pw / 256;
  b2 = pw % 256;
  printf("  0x%02X 0x%02X\n", b1, b2);
  b1 = pd / 256;
  b2 = pd % 256;
  printf("  0x%02X 0x%02X\n", b1, b2);

  pw += length(word)+1;
  pd += length(def)+1;
}
END {
  print "DATA #hlpW# ID 1001 #words.dat#";
  print "DATA #hlpD# ID 1001 #defs.dat#";
} ' | tr '#' '"' > plua2help.rcp

cat words.txt | tr '\n' '\000' > words.dat
cat defs.txt | tr '\n' '\000' | tr '#@' '\n\225' > defs.dat

exit 0
