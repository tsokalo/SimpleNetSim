extern "C"
{
#include "typedefs.h"
}

#include <cstdio>


inline size_t CompVariableLsb(u32_t value, u8_t *pBuf)
{
    size_t length = 0U;

    /* lsb_7 */
    if (value <= 0x7fff)
    {
        pBuf[0] = 0x80;
        pBuf[0] |= value & 0x7f;
        length += 1;
    }
    /* lsb_14 */
    else if (value <= 0x3fff)
    {
        pBuf[0] = 0x40;
        pBuf[0] |= value >> 8 & 0x3f;
        pBuf[1] = value & 0xff;
        length += 2;
    }
    /* lsb_21 */
    else if (value <= 0x1fffff)
    {
        pBuf[0] = 0x20;
        pBuf[0] |= value >> 16 & 0x1f;
        pBuf[1] = value >> 8 & 0xff;
        pBuf[2] = value & 0xff;
        pBuf += 3;
        length += 3;
    }
    /* lsb_29 */
    else if (value <= 0x0fffffff)
    {
        pBuf[0] = 0x10;
        pBuf[0] |= value >> 24 & 0x0f;
        pBuf[1] = value >> 16 & 0xff;
        pBuf[2] = value >> 8 & 0xff;
        pBuf[3] = value & 0xff;
        pBuf += 4;
        length += 4;
    }
    /* full_offset */
    else
    {
        pBuf[0] = 0xff;
        pBuf[1] = value >> 24 & 0xff;
        pBuf[2] = value >> 16 & 0xff;
        pBuf[3] = value >> 8 & 0xff;
        pBuf[4] = value & 0xff;
        length += 5;
    }

    return length;
} /* CompVariableLsb */
