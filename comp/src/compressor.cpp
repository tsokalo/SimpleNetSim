#include "compressor.hpp"    /* Current header */


#include "lsb.hpp"

#include <cstring>
#include <cassert>


fbcd::Compressor::Compressor(u8_t optimisticCnt, u16_t sensitivity)
    : optimisticCnt { optimisticCnt }, isOptimistic { true },
      optimisticTo { optimisticCnt }, sensitivity { sensitivity },
      sn { 0U }, ucBytes { 0U }, coBytes { 0U },
      priorityPrev { 0U }, priorityCurr { 0U }
{
    csrc_table_init(&this->probabilityTable, 0);
    this->probabilityTable.oaCnt        = this->optimisticCnt;
    this->probabilityTable.oaCntInit    = this->optimisticCnt;
} /* Compressor */

fbcd::Compressor::~Compressor()
{

} /* ~Compressor */

void fbcd::Compressor::Update(u32_t priority, pf_t &probabilities)
{
    isOptimistic   = this->optimisticTo ? true : false;

    /* decrease optimistic "time-out" */
    this->optimisticTo = this->optimisticTo ? --this->optimisticTo : 0U;;

    ++this->sn;

    /* priority */
    this->priorityPrev = this->priorityCurr;
    this->priorityCurr = priority;
    if (this->priorityCurr != this->priorityPrev)
    {
        isOptimistic = true;
        this->optimisticTo = this->optimisticCnt;
    }

    /* priority */
    if (priority & 0x80000000)
    {
        char msg[500] = { 0 };
        sprintf(msg, "Priority out of bounds with value 0x%0x!", priority);
        throw CompressionError { msg };
    }

    u8_t items[64] = { 0x00 };
    size_t i = 0U;
    for (auto p : probabilities)
    {
        //printf("%d - %f (%u)\n", p.first, p.second, (u32_t)(0.403 * 10000000) & 0xffffff);
        u32_t item = (p.first << 24 & 0xff000000)
            | (static_cast<u32_t>(p.second * 10000000) & 0xffffff);
        //printf("%d (%0x)\n", item, item);
        items[i++] = item >> 24 & 0xff;
        items[i++] = item >> 16 & 0xff;
        items[i++] = item >> 8 & 0xff;
        items[i++] = item & 0xff;
    }
    //printf("%0x %0x %0x\n", *(u32_t*)(items), *(u32_t*)(items + 4), *(u32_t*)(items + 8));
    csrc_table_evaluate_field(&this->probabilityTable,
        reinterpret_cast<u32_t*>(items), probabilities.size());
    this->ucBytes += probabilities.size() * (sizeof (short) + sizeof (double));

    /* stats */
    this->ucBytes += sizeof (u32_t);
} /* Update */

std::stringstream& fbcd::Compressor::operator >> (std::stringstream &ss)
{
    if (this->isOptimistic)
    {
        /* set optimistic flag & priority*/
        //printf("%u\n", (0x80000000 | this->priorityCurr));

        /* flags */
        u8_t flags = 0x80;

#if 0
        ss << (0x80000000 | this->priorityCurr);
#else
        /*ss << static_cast<char>(0x80 | (this->priorityCurr >> 24 & 0xff));
        ss << static_cast<char>(this->priorityCurr >> 16 & 0xff);
        ss << static_cast<char>(this->priorityCurr >> 8 & 0xff);
        ss << static_cast<char>(this->priorityCurr & 0xff);*/

        u8_t buf[5] = { 0U };
        size_t len = 0U;
        if (this->priorityCurr > this->priorityPrev)
        {
            if (this->priorityCurr - this->priorityPrev > this->sensitivity)
            {
                len = CompVariableLsb(this->priorityCurr - this->priorityPrev, &buf[0]);
            }
            else
            {
                flags = 0x00;
                len = 0U;
            }
        }
        else
        {
            if (this->priorityPrev - this->priorityCurr > this->sensitivity)
            {
                flags |= 0x40;
                len = CompVariableLsb(this->priorityPrev - this->priorityCurr, &buf[0]);
            }
            else
            {
                flags = 0x00;
                len = 0U;
            }
        }

        /* flags */
        ss << flags;
        /*if (buf[0] == 0x20)
        {
            ss << (u8_t)0x20;
        }*/
        for (size_t i = 0U; i < len; ++i)
        {
            ss << (u8_t)buf[i];
        }
#endif

        /* stats */
        this->coBytes += len + 1;
    }
    else
    {
        /* flags */
        ss << 0x00;

        /* stats */
        this->coBytes += 1U;
    }

    /* probability compression */
    const size_t BUF_SZ = 64U;
    u8_t buf[BUF_SZ] = { 0U };
    u8_t len = 0U;
    if (list_csrc_compress(&this->probabilityTable, buf, BUF_SZ, &len))
    {
        assert(false);
    }

    for (size_t j = 0U; j < len; ++j)
    {
        //printf("%0x ", buf[j]);
        ss << (u8_t)buf[j];
    }
    //printf("\n");
    this->coBytes += len;

    return ss;
} /* operator << */

float fbcd::Compressor::GetCompressionRate() const
{
    return 1.0f - this->coBytes / static_cast<float>(this->ucBytes);
} /* GetCompressionRate */

fbcd::Compressor::CompressionError::CompressionError(const char* msg)
    : std::exception { }, msg { msg }
{

} /* CompressionError */

fbcd::Compressor::CompressionError::~CompressionError()
{

} /* ~CompressionError */

const char* fbcd::Compressor::CompressionError::what() const throw()
{
    return this->msg.c_str();
} /* what */

std::stringstream& fbcd::operator << (std::stringstream &ss, Compressor &c)
{
    return c >> ss;
} /* operator << */
