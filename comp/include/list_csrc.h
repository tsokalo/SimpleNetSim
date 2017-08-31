#ifndef __LIST_CSRC_H__
#define __LIST_CSRC_H__


#include "typedefs.h"


#define MAX_CC      16
#define MAX_IDX     16
#define CO_BUF_LEN  529


enum Confidence
{
    UNAVAILABLE = -1,
    OMITTED,
    UNSENT,
    PROCESSING,
    CONFIDENT
}; /* Confidence */

typedef struct CSRCCompressionTable
{
    int indexField[MAX_CC];

    int topIndex;
    int firstOmitted;
    u8_t cc_value;
    u8_t oaCnt, oaCntInit;

    struct CSRCSet
    {
        enum Confidence confidence;
        u32_t       item;
        u8_t        refCnt;
        u8_t        oaLeft;
    } CSRCs[MAX_IDX];

    yesno_t initialised;
} CSRCCompressionTable_t;

#ifdef NDEBUG
#define csrc_table_sane
#else
yesno_t csrc_table_sane(CSRCCompressionTable_t const* const self);
#endif

void csrc_table_init(CSRCCompressionTable_t *pTable,
                     u8_t                   oaCnt);

void csrc_table_reinit(CSRCCompressionTable_t *pTable);

yesno_t csrc_table_is_initialised(CSRCCompressionTable_t const* const pTable);

int csrc_table_evaluate_field(CSRCCompressionTable_t    *pTable,
                              u32_t                     *items,
                              u8_t                      cc_value);

void csrc_table_set_confidencies(CSRCCompressionTable_t *pTable,
                                 enum Confidence        confidency);

#define CSRC_TABLE_RESET_CONFIDENCIES(table)    \
    csrc_table_set_confidencies(table, UNSENT)

#define CSRC_TABLE_EMPTY(table) csrc_table_init(table)

typedef struct
{
    int indices[MAX_CC];
    u32_t CSRCs[MAX_IDX];
    u8_t cc_value;
} CSRCDecompressionTable_t;

#define CSRC_DECOMP_TABLE_ITEM_SAFE_PUSH(pTable, index, item)   \
    pTable->CSRCs[index] = item

void csrc_decomp_init(CSRCDecompressionTable_t *pTable);

#define CSRC_DECOMP_LIST_LEN(pTable) ((u16_t)pTable->cc_value * 4U)

u8_t csrc_decomp_table_assemble_list(CSRCDecompressionTable_t   *pTable,
                                     u32_t                      *pListOut);

int list_csrc_compress_new(CSRCCompressionTable_t *pTable,
                           u8_t                   minBufItemSize,
                           u8_t                   **ppBufOut,
                           u8_t                   *pBufSzOut,
                           u8_t                   *pCoListLenOut);

int list_csrc_compress(CSRCCompressionTable_t *pTable,
                       u8_t                   *pBuf,
                       u32_t                  bufSz,
                       u8_t                   *pCoListLenOut);

int list_csrc_length_comp(CSRCCompressionTable_t *pTable);

#define LIST_CSRC_LENGTH_DECOMP(cc_value) (cc_value * sizeof (u32_t))

int list_csrc_decompress(CSRCDecompressionTable_t *pTable,
                         u8_t                     *pBuf,
                         u32_t                    bufSz,
                         u16_t                    *pBytesReadOut);

int list_csrc_free_buf(u8_t *pBuf);


#endif /* __LIST_CSRC_H__ */
