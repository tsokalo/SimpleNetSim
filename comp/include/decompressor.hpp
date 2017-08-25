#ifndef __DECOMPRESSOR_HPP__
#define __DECOMPRESSOR_HPP__


#include "property.hpp"

extern "C"
{
#include "typedefs.h"
}

#include <cstdio>
#include <exception>
#include <string>
#include <sstream>


namespace fbcd
{
    class Decompressor
    {
    public:
        Decompressor();

        virtual ~Decompressor();

        virtual Decompressor& operator << (std::stringstream &ss);

        class DecompressionError : public std::exception
        {
        public:
            DecompressionError(const char* msg);

            virtual ~DecompressionError();

            virtual const char* what() const throw();

        protected:
            std::string msg;
        }; /* DecompressionError */

    public:
        const diversity::Property<u32_t, Decompressor> Priority;
    }; /* Decompressor */

    Decompressor& operator >> (Decompressor &d, std::stringstream &ss);
}; /* fbcd */


#endif /* __DECOMPRESSOR_HPP__ */
