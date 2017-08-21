#include "../include/compressor.hpp"    /* Current header */


#include "../include/lsb.hpp"

#include <cstring>
#include <cassert>


fbcd::Compressor::Compressor(u8_t optimisticCnt)
    : optimisticCnt { optimisticCnt }, isOptimistic { true },
      optimisticTo { optimisticCnt }, sn { 0U }, ucBytes { 0U }, coBytes { 0U },
      priorityPrev { 0U }, priorityCurr { 0U }
{

} /* Compressor */

fbcd::Compressor::~Compressor()
{

} /* ~Compressor */

void fbcd::Compressor::Update(u32_t priority)
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
            len = CompVariableLsb(this->priorityCurr - this->priorityPrev, &buf[0]);
        }
        else
        {
            flags |= 0x40;
            len = CompVariableLsb(this->priorityPrev - this->priorityCurr, &buf[0]);
        }

        /* flags */
        ss << flags;
        for (size_t i = 0U; i < len; ++i)
        {
            ss << buf[i];
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
    c >> ss;
} /* operator << */
