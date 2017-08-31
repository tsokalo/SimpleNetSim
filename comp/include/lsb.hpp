extern "C"
{
#include "typedefs.h"
}

#include <cstdio>
#include <cassert>


inline size_t CompVariableLsb(u32_t value, u8_t *pBuf)
{
    size_t length = 0U;

    /* lsb_7 */
    if (value <= 0x7f)
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
        pBuf[0] = 0x0f;
        pBuf[1] = value >> 24 & 0xff;
        pBuf[2] = value >> 16 & 0xff;
        pBuf[3] = value >> 8 & 0xff;
        pBuf[4] = value & 0xff;
        length += 5;
    }

    return length;
} /* CompVariableLsb */

inline u32_t DecompVariableLsb(const u8_t *pBuf, size_t *pLengthOut)
{
    u32_t value = 0U;

    /* lsb_7 */
    if ((pBuf[0] & 0x80) ==  0x80)
    {
        value = pBuf[0] & 0x7f;
        *pLengthOut = 1U;
    }
    /* lsb_14 */
    else if ((pBuf[0] & 0xc0) == 0x40)
    {
        value = (pBuf[0] << 8 & 0x3f00)
            | (pBuf[1] & 0xff);
        *pLengthOut = 2U;
    }
    /* lsb_21 */
    else if ((pBuf[0] & 0xe0) == 0x20)
    {
        value = (pBuf[0] << 16 & 0x1f0000)
            | (pBuf[1] << 8 & 0xff00)
            | (pBuf[2] & 0xff);
        *pLengthOut = 3U;
    }
    /* lsb_29 */
    else if ((pBuf[0] & 0xf0) == 0x10)
    {
        value = (pBuf[0] << 24 & 0x0f000000)
            | (pBuf[1] << 16 & 0xff0000)
            | (pBuf[2] << 8 & 0xff00)
            | (pBuf[3] & 0xff);
            *pLengthOut = 4U;
    }
    /* full_offset */
    else
    {
        assert(pBuf[0] == 0x0f);
        value = (pBuf[1] << 24 & 0xff000000)
            | (pBuf[2] << 16 & 0xff0000)
            | (pBuf[3] << 8 & 0xff00)
            | (pBuf[4] & 0xff);
        *pLengthOut = 5U;
    }

    return value;
} /* DecompVariableLsb */
