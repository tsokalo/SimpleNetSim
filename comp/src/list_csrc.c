#include "list_csrc.h"  /* Current header */

#include <assert.h>
#include <stdlib.h>


#define LARGE_INDICES(pTable) (pTable->cc_value > 7 || pTable->topIndex > 7 ? YES : NO)

#define ROHC2_CEIL(x) ((unsigned)((((x) - (unsigned)(x)) > 0.0f) > 0 ? (x) + 1.0f : (x)))

static void csrc_table_ommit(CSRCCompressionTable_t *pTable,
                             u8_t                   index)
{
    assert(index < MAX_IDX);

    if (pTable->indexField[index] != -1)
    {
        if (pTable->CSRCs[pTable->indexField[index]].refCnt)
        {
            if (pTable->CSRCs[pTable->indexField[index]].refCnt == 1U)
            {
                pTable->CSRCs[pTable->indexField[index]].confidence = OMITTED;

                /* manage firstOmitted field */
                if (pTable->firstOmitted == -1)
                {
                    pTable->firstOmitted = pTable->indexField[index];
                }
                else if (pTable->firstOmitted > pTable->indexField[index])
                {
                    pTable->firstOmitted = pTable->indexField[index];
                }
            }

            pTable->CSRCs[pTable->indexField[index]].refCnt -= 1;
            pTable->indexField[index] = -1;
        }
        else
        {
            pTable->CSRCs[pTable->indexField[index]].confidence = OMITTED;
            /* manage firstOmitted field */
            if (pTable->firstOmitted == -1)
            {
                pTable->firstOmitted = pTable->indexField[index];
            }
            else if (pTable->firstOmitted > pTable->indexField[index])
            {
                pTable->firstOmitted = pTable->indexField[index];
            }
        }
    }
} /* csrc_table_ommit */

static int csrc_table_find_or_insert(CSRCCompressionTable_t *pTable,
                                     u32_t                  item)
{
    u8_t i = 0U;

    /* find item index in set if it's in it */
    for (i = 0U; i < pTable->topIndex; ++i)
    {
        if (pTable->CSRCs[i].item == item)
        {
            if (pTable->CSRCs[i].confidence == OMITTED)
            {
                pTable->CSRCs[i].confidence = CONFIDENT;
            }
            ++pTable->CSRCs[i].refCnt;
            return i;
        }
    }

    /* otherwise add the new item */
    /* insert into the first unused field */
    if (pTable->topIndex < MAX_IDX)
    {
        pTable->CSRCs[pTable->topIndex].item         = item;
        pTable->CSRCs[pTable->topIndex].confidence   = UNSENT;
        pTable->CSRCs[pTable->topIndex].refCnt = 1U;
        ++pTable->topIndex;
        return pTable->topIndex - 1;
    }
    /* if set is full, replace an omitted element */
    else if (pTable->firstOmitted > -1)
    {
        u8_t c = (u8_t)pTable->firstOmitted;

        assert(pTable->firstOmitted < MAX_IDX);

        pTable->CSRCs[pTable->firstOmitted].item    = item;
        pTable->CSRCs[pTable->firstOmitted].oaLeft  = pTable->oaCnt;
        /* find next omitted */
        for (; pTable->firstOmitted < pTable->topIndex; ++pTable->firstOmitted)
        {
            if (pTable->CSRCs[pTable->firstOmitted].confidence == OMITTED)
            {
                pTable->CSRCs[pTable->firstOmitted].item         = item;
                pTable->CSRCs[pTable->firstOmitted].confidence   = UNSENT;
                break;
            }
        }

        /* no other omitted found */
        pTable->firstOmitted = -1;
        return c;
    }
    /* the set is full */
    else
    {
        return -1;
    }
} /* csrc_table_find_or_insert */

void csrc_table_reinit(CSRCCompressionTable_t *pTable)
{
    assert(pTable->initialised);

    pTable->initialised = NO;
    csrc_table_init(pTable, pTable->oaCntInit);
}


void csrc_table_init(CSRCCompressionTable_t *pTable,
                     u8_t                   oaCnt)
{
    u8_t i = 0U;

    /*assert(!pTable->initialised);*/

    pTable->topIndex        = 0;
    pTable->firstOmitted    = -1;
    pTable->cc_value        = 0U;
    pTable->oaCnt           = oaCnt;
    pTable->oaCntInit       = oaCnt;

    for (i = 0U; i < MAX_CC; ++i)
    {
        pTable->indexField[i] = -1;
    }

    for (i = 0U; i < MAX_IDX; ++i)
    {
        pTable->CSRCs[i].confidence = UNAVAILABLE;
        pTable->CSRCs[i].item       = 0LU;
        pTable->CSRCs[i].refCnt     = 0U;
        pTable->CSRCs[i].oaLeft     = oaCnt;
    }

    pTable->initialised = YES;

    assert((pTable));
} /* csrc_table_init */

yesno_t csrc_table_is_initialised(CSRCCompressionTable_t const* const self)
{
    if (self->initialised) {
        assert((self));
    }

    return (self->initialised);
} /* csrc_table_is_initialised */


int csrc_table_evaluate_field(CSRCCompressionTable_t    *pTable,
                              u32_t                     *items,
                              u8_t                      cc_value)
{
    int rc = 0;

    assert(cc_value < MAX_CC);

    if (cc_value < pTable->cc_value)
    {
        for (; pTable->cc_value > cc_value; --pTable->cc_value)
        {
            csrc_table_ommit(pTable, pTable->cc_value - 1);
        }
        rc = 1;
    }

    for (pTable->cc_value = 0U; pTable->cc_value < cc_value; ++pTable->cc_value)
    {
        /* if new item was introduced */
        if (pTable->CSRCs[pTable->indexField[pTable->cc_value]].item
            != NTOHL(items[pTable->cc_value]))
        {
            rc = 1;

            csrc_table_ommit(pTable, pTable->cc_value);
            pTable->indexField[pTable->cc_value]
                = csrc_table_find_or_insert(pTable, NTOHL(items[pTable->cc_value]));

            if (pTable->indexField[pTable->cc_value] < 0)
            {
                return -1;
            }
        }
    }

    return rc;
} /* csrc_table_evaluate_field */

void csrc_table_set_confidencies(CSRCCompressionTable_t *pTable,
                                 enum Confidence        confidency)
{
    u8_t i = 0U;

    for (i = 0U; i < pTable->topIndex; ++i)
    {
        pTable->CSRCs[i].confidence = confidency;
    }
} /* csrc_table_set_confidencies */

void csrc_decomp_init(CSRCDecompressionTable_t *pTable)
{
    u8_t i = 0U;

    for (i = 0U; i < MAX_CC; ++i)
    {
        pTable->indices[i]  = -1;
        pTable->CSRCs[i]    = 0x00000000;
    }

    pTable->cc_value = 0U;
} /* csrc_decomp_init */

u8_t csrc_decomp_table_assemble_list(CSRCDecompressionTable_t   *pTable,
                                     u32_t                      *pListOut)
{
    u8_t i = 0U;

    assert(pTable->cc_value <= MAX_CC);

    for (i = 0U; i < pTable->cc_value; ++i)
    {
        if (pTable->indices[i] != -1)
        {
            pListOut[i] = pTable->CSRCs[pTable->indices[i]];
        }
        else
        {
            return i;
        }
    }

    return i;
} /* csrc_decomp_table_assemble_list */

int list_csrc_compress_new(CSRCCompressionTable_t   *pTable,
                           u8_t                     minBufItemSize,
                           u8_t                     **ppBufOut,
                           u8_t                     *pBufSzOut,
                           u8_t                     *pCoListLenOut)
{
    /*assert(*ppBufOut == 0);*/

    /* if there are items in the table */
    /* and more then min buf size */
    if (pTable->cc_value > 0U && pTable->cc_value > minBufItemSize)
    {
        *pBufSzOut = sizeof (u32_t) * pTable->cc_value
            + (LARGE_INDICES(pTable)  ? pTable->cc_value + 1
            : (u8_t)ROHC2_CEIL(pTable->cc_value / 2.0f)) + 1;
        *ppBufOut = (u8_t*)malloc(*pBufSzOut);

        if (*ppBufOut == 0)
        {
            return -1;
        }

        return list_csrc_compress(pTable, *ppBufOut, *pBufSzOut, pCoListLenOut);
    }
    /* allocate min buf size */
    else
    {
        /* CC == 0 */
        if (pTable->cc_value == 0U)
        {
            *pBufSzOut = 1U;
            *ppBufOut = (u8_t*)malloc(*pBufSzOut * sizeof (u8_t));

            if (*ppBufOut == 0)
            {
                return -1;
            }

            return list_csrc_compress(pTable, *ppBufOut, *pBufSzOut, pCoListLenOut);
        }
        else
        {
            /* if don't allocate anything */
            if (minBufItemSize == 0U)
            {
                *ppBufOut = 0;
                *pCoListLenOut = 0U;
                return 0;
            }

            *pBufSzOut = sizeof (u32_t) * minBufItemSize
                + (LARGE_INDICES(pTable) ? minBufItemSize + 1
                : (u8_t)ROHC2_CEIL(minBufItemSize / 2.0f)) + 1;
            *ppBufOut = (u8_t*)malloc(*pBufSzOut * sizeof (u8_t));

            if (*ppBufOut == 0)
            {
                return -1;
            }

            return list_csrc_compress(pTable, *ppBufOut, *pBufSzOut, pCoListLenOut);
        }
    }
} /* list_csrc_compress_new */

int list_csrc_compress(CSRCCompressionTable_t   *pTable,
                       u8_t                     *pBuf,
                       u32_t                    bufSz,
                       u8_t                     *pCoListLenOut)
{
    u8_t i = 0U;
    u8_t j = 0U;
    u8_t unsentCnt = 0U;

    assert(bufSz > 0U);

    /* CC == 0 */
    if (pTable->cc_value == 0U)
    {
        if (bufSz == 0U)
        {
            return -1;
        }

        *pCoListLenOut = 1U;
        pBuf[0] = 0U;
        return 0;
    }

    /* 8-bit XIs */
    if (LARGE_INDICES(pTable))
    {
        /* estimate needed max buffer size */
        if (pTable->cc_value * 4U + pTable->cc_value + 1U  > bufSz)
        {
            return -1;
        }

        for (i = 0U; i < pTable->cc_value; ++i)
        {
            pBuf[i + 1] = (pTable->CSRCs[pTable->indexField[i]].confidence
                == UNSENT ? 0x80 : 0x00) | (pTable->indexField[i] & 0x0f);
        }
    }
    /* 4-bit XIs */
    else
    {
        /* estimate needed max buffer size */
        if (pTable->cc_value * 4 + ROHC2_CEIL(pTable->cc_value / 2.0f) + 1  > bufSz)
        {
            return -1;
        }

        for (i = 0U; i < ROHC2_CEIL(pTable->cc_value / 2.0f); ++i)
        {
            /* upper nibble */
            if (pTable->indexField[i * 2] == -1)
            {
                return -2;
            }
            pBuf[i + 1] = ((pTable->CSRCs[pTable->indexField[i * 2]].confidence
                == UNSENT ? 0x08 : 0x00) | ((pTable->indexField[i * 2]) & 0x07)) << 4;
            /* is being sent right now */
            if (pTable->CSRCs[pTable->indexField[i * 2]].confidence == UNSENT)
            {
                pTable->CSRCs[pTable->indexField[i * 2]].confidence = PROCESSING;
            }

            /* do padding */
            if (pTable->indexField[i * 2 + 1] == -1)
            {
                pBuf[i + 1] &= 0xf0;
            }
            else
            {
                pBuf[i + 1] |= (pTable->CSRCs[pTable->indexField[i * 2 + 1]].confidence
                    == UNSENT ? 0x08 : 0x00) | ((pTable->indexField[i * 2 + 1 ]) & 0x07);
                /* is being sent right now */
                if (pTable->CSRCs[pTable->indexField[i * 2 + 1]].confidence == UNSENT)
                {
                    pTable->CSRCs[pTable->indexField[i * 2 + 1]].confidence = PROCESSING;
                }
            }
        }
    }
    *pCoListLenOut = i + 1;

    /* write item list */
    for (j = 0U; j < pTable->cc_value; ++j)
    {
        if (pTable->CSRCs[pTable->indexField[j]].confidence == UNSENT
            || pTable->CSRCs[pTable->indexField[j]].confidence == PROCESSING)
        {
            *(u32_t*)(pBuf + i + 1 + unsentCnt * 4)
                = HTONL(pTable->CSRCs[pTable->indexField[j]].item);
            pTable->CSRCs[pTable->indexField[j]].confidence = CONFIDENT;
            ++unsentCnt;
        }
    }
    *pCoListLenOut += unsentCnt * 4;

    /* encoding field */
    pBuf[0] = (LARGE_INDICES(pTable)  ? 0x10 : 0x00)
        | (pTable->cc_value & 0x0f);

    assert(*pCoListLenOut > 0U);

    /* optimistic approach */
    for (j = 0U; j < pTable->cc_value; ++j)
    {
        if (pTable->CSRCs[j].oaLeft > 0
            && pTable->CSRCs[j].confidence == CONFIDENT)
        {
            --(pTable->CSRCs[j].oaLeft);
            pTable->CSRCs[j].confidence = UNSENT;
        }
    }

    return 0;
} /* list_csrc_compress */

int list_csrc_length_comp(CSRCCompressionTable_t *pTable)
{
    u8_t i = 0U;
    int len = 1;    /* control field */

    if (LARGE_INDICES(pTable))
    {
        /* large XIs */
        len += pTable->cc_value;
    }
    else
    {
        /* small XIs */
        len += ROHC2_CEIL(pTable->cc_value / 2.0f);
    }

    /* unsent items */
    for (i = 0U; i < pTable->cc_value; ++i)
    {
        len += pTable->CSRCs[pTable->indexField[i]].confidence
            == UNSENT ? sizeof (u32_t) : 0;
    }

    return len;
} /* list_csrc_length_comp */

int list_csrc_decompress(CSRCDecompressionTable_t   *pTable,
                         u8_t                       *pBuf,
                         u32_t                      bufSz,
                         u16_t                      *pBytesReadOut)
{
#define m pTable->cc_value
    u8_t i = 0U;
    pTable->cc_value = 0U;
    *pBytesReadOut = 1U;

    assert(bufSz > 0U);

    /* read cc_value */
    m = pBuf[0] & 0x0f;

    /* CC == 0 */
    if (m == 0U)
    {
        return 0;
    }

    if (m > MAX_CC)
    {
        return -3;
    }

    /* 8-bit XIs */
    if ((pBuf[0] & 0x10) != 0U)
    {
        u8_t *pItemsStart  = pBuf + 1 + m;
        u8_t itemsRead    = 0U;

        if (bufSz < (u32_t)(m + 1))
        {
            return -4;
        }

        *pBytesReadOut += m;
        for (i = 0U; i < m; ++i)
        {
            u8_t okt = pBuf[i + 1];

            pTable->indices[i] = okt & 0x0f;

            /* if item available */
            if ((okt & 0x80) != 0U)
            {
                if ((u32_t)(m + 2 + itemsRead * sizeof (u32_t)) > bufSz)
                {
                    return -4;
                }

                CSRC_DECOMP_TABLE_ITEM_SAFE_PUSH(pTable, pTable->indices[i],
                    ((u32_t*)pItemsStart)[itemsRead]);
                ++itemsRead;
                *pBytesReadOut += 4U;
            }
        }
    }
    /* 4-bit XIs */
    else
    {
        u8_t ciel = (u8_t)ROHC2_CEIL(m / 2.0f);
        u8_t *pItemsStart  = pBuf + 1 + ciel;
        u8_t itemsRead    = 0U;

        *pBytesReadOut += ciel;
        for (i = 0U; i < m / 2.0f; ++i)
        {
            u8_t okt = pBuf[i + 1];

            pTable->indices[i * 2]      = (okt & 0x70) >> 4;
            pTable->indices[i * 2 + 1]  = okt & 0x07;

            /* upper nibble */
            /* if item available */
            if ((okt & 0x80) == 0x80)
            {
                if ((u32_t)(ciel + 2 + itemsRead * sizeof (u32_t)) > bufSz)
                {
                    return -4;
                }

                CSRC_DECOMP_TABLE_ITEM_SAFE_PUSH(pTable, pTable->indices[i * 2],
                    ((u32_t*)pItemsStart)[itemsRead]);
                ++itemsRead;
                *pBytesReadOut += 4U;
            }

            /* lower nibble */
            /* if item available */
            if ((okt & 0x08) == 0x08)
            {
                if ((u32_t)(ciel + 2 + itemsRead * sizeof (u32_t)) > bufSz)
                {
                    return -4;
                }

                CSRC_DECOMP_TABLE_ITEM_SAFE_PUSH(pTable, pTable->indices[i * 2 + 1],
                    ((u32_t*)pItemsStart)[itemsRead]);
                ++itemsRead;
                *pBytesReadOut += 4U;
            }
        }
#if 0
        /* last byte */
        /* if odd */
        if ((m % 2) == 1)
        {
            /* only read the upper nibble */
            pTable->indices[i * 2 + 1] = (pBuf[i + 1] >> 4) & 0x07;
            /* if item available */
            if ((pBuf[i + 1] & 0x80) != 0U)
            {
                CSRC_DECOMP_TABLE_ITEM_SAFE_PUSH(pTable, pTable->indices[i * 2 + 1],
                    pItemsStart[itemsRead]);
                ++itemsRead;
                *pBytesReadOut += 4U;
            }
        }
#endif
    }
    return 0;
#undef m
} /* list_csrc_decompress */

int list_csrc_free_buf(u8_t *pBuf)
{
    free(pBuf);
    pBuf = (u8_t*)NULL;

    return 0;
} /* list_csrc_free */

#undef ROHC2_CEIL
#undef LARGE_INDICES
