#include "decompressor.hpp"  /* Current header */

#include "lsb.hpp"


fbcd::Decompressor::Decompressor()
    : Priority { 0U }
{

} /* Decompressor */

fbcd::Decompressor::~Decompressor()
{

} /* ~Decompressor */

fbcd::Decompressor& fbcd::Decompressor::operator << (std::stringstream &ss)
{
    const u8_t *pBuf = reinterpret_cast<const u8_t*>(ss.str().data());
    size_t len = ss.str().size();

    /* flags */
    u8_t flags = pBuf[0];
    size_t variableLen = 0U;

    /* optimistic */
    if (flags & 0x80)
    {
        /* priority */
        this->Priority(flags & 0x40
            ? this->Priority - DecompVariableLsb(pBuf + 1, &variableLen)
            : this->Priority + DecompVariableLsb(pBuf + 1, &variableLen));
    }
    else
    {
        /* nothing to do for priority */
    }

    /* probabilities */
    u8_t *pProbBuf = (u8_t*)(pBuf) + variableLen + 1;
    u16_t bytesRead = 0U;
    if (list_csrc_decompress(&this->probabilityTable, pProbBuf, len - variableLen, &bytesRead))
    {
        assert(false);
    }

    const size_t BUF_SZ = 64U;
    u8_t pBufOut[BUF_SZ] = { 0x00 };
    csrc_decomp_table_assemble_list(&this->probabilityTable, (u32_t*)pBufOut);

    this->Probability().clear();
    for (size_t j = 0U; j < this->probabilityTable.cc_value * sizeof (u32_t);
        j += sizeof (u32_t))
    {
        short index = pBufOut[j] & 0xff;
        double prob = (pBufOut[j + 1] << 16 | pBufOut[j + 2] << 8
            | pBufOut[j + 3]) / 10000000.0;
        this->Probability().insert( { index, prob } );
    }

    return *this;
} /* operator >> */

fbcd::Decompressor::DecompressionError::DecompressionError(const char* msg)
    : std::exception { }, msg { msg }
{

} /* DecompressionError */

fbcd::Decompressor::DecompressionError::~DecompressionError()
{

} /* ~DecompressionError */

const char* fbcd::Decompressor::DecompressionError::what() const throw()
{
    return this->msg.c_str();
} /* what */

fbcd::Decompressor& fbcd::operator >> (Decompressor &d, std::stringstream &ss)
{
    return d << ss;
} /* operator >> */
