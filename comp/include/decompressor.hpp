#ifndef __DECOMPRESSOR_HPP__
#define __DECOMPRESSOR_HPP__


extern "C"
{
#include "typedefs.h"
}


namespace fbcd
{
    class Decompressor
    {
    public:
        Decompressor();

        virtual ~Decompressor();

    protected:
        u32_t priority;
    }; /* Decompressor */
}; /* fbcd */


#endif /* __DECOMPRESSOR_HPP__ */
