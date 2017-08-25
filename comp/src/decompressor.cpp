#include "../include/decompressor.hpp"  /* Current header */

#include "../include/lsb.hpp"


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

    /* flags */
    u8_t flags = pBuf[0];

    /* optimistic */
    if (flags & 0x80)
    {
        /* priority */
        this->Priority(flags & 0x40
            ? this->Priority - DecompVariableLsb(pBuf + 1)
            : this->Priority + DecompVariableLsb(pBuf + 1));
    }
    else
    {
        /* nothing to do for priority */
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
