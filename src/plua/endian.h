#ifndef _ENDIAN_H
#define _ENDIAN_H

#define ByteSwap16(n) ( ((((UInt16) n) << 8) & 0xFF00) | \
                        ((((UInt16) n) >> 8) & 0x00FF) )

#define ByteSwap32(n) ( ((((UInt32) n) << 24) & 0xFF000000l) |    \
                        ((((UInt32) n) <<  8) & 0x00FF0000l) |    \
                        ((((UInt32) n) >>  8) & 0x0000FF00l) |    \
                        ((((UInt32) n) >> 24) & 0x000000FFl) )
#endif
