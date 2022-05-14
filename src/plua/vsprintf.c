#ifdef LINUX

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define SEC(x)

typedef char Int8;
typedef short Int16;
typedef int Int32;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;

typedef union {
  double d;
  UInt32 ul[2];
} FlpCompDouble;

#else

#include "p.h"

#endif

#define ZEROPAD 0x0001      // pad with zero
#define SIGN    0x0002      // unsigned/signed long
#define PLUS    0x0004      // show plus
#define SPACE   0x0008      // space if plus
#define LEFT    0x0010      // left justified
#define SPECIAL 0x0020      // 0x
#define LARGE   0x0040      // use 'ABCDEF' instead of 'abcdef'
#define EXP     0x0080      // notacao exponencial com e
#define EXPE    0x0100      // notacao exponencial com E
#define NOZERO  0x0200      // remove zeros a direita

#define NUM_DIGITS      15
#define MIN_FLOAT       6
#define ROUND_FACTOR    1.0000000000000005

static double pow1[] = {1e256,1e128,1e064,1e032,1e016,1e008,1e004,1e002,1e001};
static double pow2[] = {1e-256,1e-128,1e-064,1e-032,1e-016,1e-008,1e-004,1e-002,1e-001};

static int skip_atoi(const char **s) SEC("aux");
static size_t strnlen(const char * s, size_t count) SEC("aux");
static int do_div(long *n, unsigned int base) SEC("aux");
static char *adjust(char *buf, int len, char *out, int size, int flags);
static char *my_itoa(char *buf, long num, int base, int size, int precision, int flags) SEC("aux");
static char *my_ftoa(char *buf, double f, int size, int precision, int flags) SEC("aux");
static int my_vsprintf(char *buf, const char *fmt, va_list args) SEC("aux");

static int skip_atoi(const char **s)
{
  int i = 0;

  while (isdigit(**s))
    i = i*10 + *((*s)++) - '0';
  return i;
}

static size_t strnlen(const char * s, size_t count)
{
  const char *sc;

  for (sc = s; count-- && *sc != '\0'; ++sc)
    /* nothing */;
  return sc - s;
}

static int do_div(long *n, unsigned int base)
{
  int res;
  res = *n % base;
  *n = *n / base;
  return res;
}

static char *my_itoa(char *buf, long num, int base, int size, int precision, int flags)
{
  char c, sign, tmp[66];
  char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  int i;

  if (flags & LARGE)
    digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (flags & LEFT)
    flags &= ~ZEROPAD;
  if (base < 2 || base > 36)
    return 0;
  c = (flags & ZEROPAD) ? '0' : ' ';
  sign = 0;
  if (flags & SIGN) {
    if (num < 0) {
      sign = '-';
      num = -num;
      size--;
    } else if (flags & PLUS) {
      sign = '+';
      size--;
    } else if (flags & SPACE) {
      sign = ' ';
      size--;
    }
  }
  if (flags & SPECIAL) {
    if (base == 16)
      size -= 2;
    else if (base == 8)
      size--;
  }
  i = 0;
  if (num == 0)
    tmp[i++]='0';
  else while (num != 0)
    tmp[i++] = digits[do_div(&num,base)];
  if (i > precision)
    precision = i;
  size -= precision;

  if (!(flags&(ZEROPAD+LEFT)))
    while(size-->0)
      *buf++ = ' ';
  if (sign)
    *buf++ = sign;
  if (flags & SPECIAL) {
    if (base==8)
      *buf++ = '0';
    else if (base==16) {
      *buf++ = '0';
      *buf++ = digits[33];
    }
  }
  if (!(flags & LEFT))
    while (size-- > 0)
      *buf++ = c;
  while (i < precision--)
    *buf++ = '0';
  while (i-- > 0)
    *buf++ = tmp[i];
  while (size-- > 0)
    *buf++ = ' ';
  return buf;
}

static char *adjust(char *buf, int len, char *out, int size, int flags)
{
  int i, n;

  if (!buf || len <= 0 || !out)
    return out;

  if (len >= size) {
    strncpy(out, buf, len);
    return out+len;
  }

  if (size < 0)
    size = len;

  if ((flags & LEFT) || buf[0] == '[')
    flags &= ~ZEROPAD;

  n = size - len;

  if (flags & ZEROPAD) {
    if (buf[0] == '-' || buf[0] == '+') {
      *out++ = *buf++;
      len--;
    }
    for (i = 0; i < n; i++) *out++ = '0';
    strncpy(out, buf, len);
    out += len;

  } else {
    if (!(flags & LEFT))
      for (i = 0; i < n; i++) *out++ = ' ';
    strncpy(out, buf, len);
    out += len;
    if (flags & LEFT)
      for (i = 0; i < n; i++) *out++ = ' ';
  }

  return out;
}

static char *my_ftoa(char *buf, double f, int size, int precision, int flags)
{
  FlpCompDouble dd;
  Int16 sign, exp;
  UInt32 manH, manL;
  Int16 i, e, e1, dec;
  double *pd, *pd1, r;
  char expsign, *dot, *pbuf, tmp[64];

  f *= ROUND_FACTOR;

  if (precision < 0)
    precision = 6;
  else if (precision > 15)
    precision = 15;

  if (f != 0 && !(flags & EXP)) {
    // arredonda para a precisao desejada

    r = 0.5;
    for (i = precision; i > 0; i--)
      r /= 10.0;
    if (f < 0.0)
      f -= r;
    else
      f += r;
  }

  dd.d = f;
  dec = 0;
  expsign = 0;
  dot = NULL;

#ifdef LINUX
  sign = (dd.ul[1] >> 31);
  exp = ((dd.ul[1] & 0x7ff00000) >> 20);
  manH = dd.ul[1] & 0x000fffff;
  manL = dd.ul[0];
#else
  sign = (dd.ul[0] >> 31);
  exp = ((dd.ul[0] & 0x7ff00000) >> 20);
  manH = dd.ul[0] & 0x000fffff;
  manL = dd.ul[1];
#endif

  if (exp == 0x7ff) {
    if (manH == 0 && manL == 0) {
      if (sign) {
        strcpy(tmp, "[-inf]");
        return adjust(tmp, 6, buf, size, flags);
      }
      strcpy(tmp, "[inf]");
      return adjust(tmp, 5, buf, size, flags);
    }
    strcpy(tmp, "[nan]");
    return adjust(tmp, 5, buf, size, flags);
  }

  if (exp == 0 && manH == 0 && manL == 0) {
    strcpy(tmp, "0");
    return adjust(tmp, 1, buf, size, flags);
  }

  pbuf = tmp;

  if (sign) {
    *pbuf++ = '-';
    f = -f;
  } else {
    if (flags & PLUS)
      *pbuf++ = '+';
    else if (flags & SPACE)
      *pbuf++ = ' ';
  }

  if ((UInt16)exp < 0x3ff) {
    for (e=1, e1=256, pd=pow1, pd1=pow2; e1; e1 >>= 1, ++pd, ++pd1)
      if (!(*pd1 <= f)) {
        e += e1;
        f = f * (*pd);
      }
    f = f * 10.0;
    if (e <= MIN_FLOAT && !(flags & EXP)) {
      *pbuf++ = '0';
      dot = pbuf;
      *pbuf++ = '.';
      dec = -1;
      while (--e) 
        *pbuf++ = '0';
    } else
      expsign = '-';
  } else {
    for (e=0, e1=256, pd=pow1, pd1=pow2; e1; e1>>=1, ++pd, ++pd1)
      if (*pd <= f) {
        e += e1;
        f = f * (*pd1);
      }
    if (e < NUM_DIGITS && !(flags & EXP))
      dec = e;
    else
      expsign = '+';
  }

  for (i = 0; i < NUM_DIGITS; i++, dec--) {
    UInt32 d = (UInt32)f;
    *pbuf++ = d + '0';
    if (!dec) {
      dot = pbuf;
      *pbuf++ = '.';
    }
    f = f - (double)d;
    f = f * 10.0;
  }

  if (dot) {
    if ((pbuf - dot - 1) > precision) {
      // trunca caracteres a direita

      while ((pbuf - dot - 1) > precision)
        *--pbuf = 0;
      if (pbuf[-1] == '.')
        *--pbuf = 0;
    }

    if (flags & NOZERO) {
      // remove zeros a direita

      while (pbuf[-1] == '0')
        *--pbuf = 0;
      if (pbuf[-1] == '.')
        *--pbuf = 0;
    }
  }

  if (expsign) {
    // coloca notacao exponencial
    *pbuf++ = (flags & EXPE) ? 'E' : 'e';
    *pbuf++ = expsign;
    pbuf = my_itoa(pbuf, e, 10, -1, 2, SIGN | ZEROPAD);
  }

  return adjust(tmp, pbuf - tmp, buf, size, flags);
}

static int my_vsprintf(char *buf, const char *fmt, va_list args)
{
  int i, base, len, flags;
  int field_width;  /* width of output field */
  int precision;    /* min. # of digits for integers; max number of chars for from string */
  int qualifier;    /* 'h', 'l', or 'L' for integer fields */
  unsigned long num;
  char *s, *str;
  double fl;

  for (str=buf ; *fmt ; ++fmt) {
    if (*fmt != '%') {
      *str++ = *fmt;
      continue;
    }
      
    /* process flags */
    flags = 0;
    repeat:
    ++fmt;    /* this also skips first '%' */
    switch (*fmt) {
      case '-': flags |= LEFT; goto repeat;
      case '+': flags |= PLUS; goto repeat;
      case ' ': flags |= SPACE; goto repeat;
      case '#': flags |= SPECIAL; goto repeat;
      case '0': flags |= ZEROPAD; goto repeat;
    }
    
    /* get field width */
    field_width = -1;
    if (isdigit(*fmt))
      field_width = skip_atoi(&fmt);
    else if (*fmt == '*') {
      ++fmt;
      /* it's the next argument */
      field_width = va_arg(args, int);
      if (field_width < 0) {
        field_width = -field_width;
        flags |= LEFT;
      }
    }

    /* get the precision */
    precision = -1;
    if (*fmt == '.') {
      ++fmt;  
      if (isdigit(*fmt))
        precision = skip_atoi(&fmt);
      else if (*fmt == '*') {
        ++fmt;
        /* it's the next argument */
        precision = va_arg(args, int);
      }
      if (precision < 0)
        precision = 0;
    }

    /* get the conversion qualifier */
    qualifier = -1;
    if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
      qualifier = *fmt;
      ++fmt;
    }

    /* default base */
    base = 10;

    switch (*fmt) {
    case 'c':
      if (!(flags & LEFT))
        while (--field_width > 0)
          *str++ = ' ';
      *str++ = (unsigned char) va_arg(args, int);
      while (--field_width > 0)
        *str++ = ' ';
      continue;

    case 's':
      s = va_arg(args, char *);
      if (!s)
        s = "<NULL>";

      len = strnlen(s, precision);

      if (!(flags & LEFT))
        while (len < field_width--)
          *str++ = ' ';
      for (i = 0; i < len; ++i)
        *str++ = *s++;
      while (len < field_width--)
        *str++ = ' ';
      continue;

    case 'p':
      if (field_width == -1) {
        field_width = 2*sizeof(void *);
        flags |= ZEROPAD;
      }
      str = my_itoa(str, (unsigned long) va_arg(args, void *), 16, field_width, precision, flags);
      continue;

    case 'e':
      fl = va_arg(args, double);
      str = my_ftoa(str, fl, field_width, precision, flags | EXP);
      continue;

    case 'E':
      fl = va_arg(args, double);
      str = my_ftoa(str, fl, field_width, precision, flags | EXP | EXPE);
      continue;

    case 'f':
      fl = va_arg(args, double);
      str = my_ftoa(str, fl, field_width, precision, flags);
      continue;

    case 'g':
    case 'G':
      fl = va_arg(args, double);
      str = my_ftoa(str, fl, field_width, precision, flags | NOZERO);
      continue;

    case 'n':
      if (qualifier == 'l') {
        long * ip = va_arg(args, long *);
        *ip = (str - buf);
      } else {
        int * ip = va_arg(args, int *);
        *ip = (str - buf);
      }
      continue;

    /* integer number formats - set up the flags and "break" */
    case 'o':
      base = 8;
      break;

    case 'X':
      flags |= LARGE;
    case 'x':
      base = 16;
      break;

    case 'd':
    case 'i':
      flags |= SIGN;
    case 'u':
      break;

    default:
      if (*fmt != '%')
        *str++ = '%';
      if (*fmt)
        *str++ = *fmt;
      else
        --fmt;
      continue;
    }
    if (qualifier == 'l')
      num = va_arg(args, unsigned long);
#ifndef LINUX
    else if (qualifier == 'h')
      if (flags & SIGN)
        num = va_arg(args, short);
      else
        num = va_arg(args, unsigned short);
#endif
    else if (flags & SIGN)
      num = va_arg(args, int);
    else
      num = va_arg(args, unsigned int);
    str = my_itoa(str, num, base, field_width, precision, flags);
  }
  *str = '\0';
  return str-buf;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
  return my_vsprintf(buf, fmt, args);
}

#ifdef LINUX

int my_sprintf(char * buf, const char *fmt, ...)
{
  va_list argp;
  int n;

  va_start(argp, fmt);
  n = my_vsprintf(buf, fmt, argp);
  va_end(argp);
  return n;
}

int main(int argc, char *argv[])
{
  char buf1[80], buf2[80];

  char *fmt = "% e";
  double d = 0.12345;

     sprintf(buf1, fmt, d);
  my_sprintf(buf2, fmt, d);

  printf("[%s]\n[%s]\n", buf1, buf2);
  exit(0);
}
#endif
