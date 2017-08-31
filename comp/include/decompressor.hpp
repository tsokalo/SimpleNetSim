#ifndef __DECOMPRESSOR_HPP__
#define __DECOMPRESSOR_HPP__


#include "property.hpp"

extern "C"
{
#include "typedefs.h"
#include "list_csrc.h"
}
#include "typedefs.hpp"

#include <cstdio>
#include <exception>
#include <string>
#include <sstream>


namespace fbcd
{
    /*!
     * \brief Decompression context for brr-pkt-header.
     */
    class Decompressor
    {
    public:
        /*!
         * \brief ctor.
         */
        Decompressor();

        /*!
         * \brief dtor.
         */
        virtual ~Decompressor();

        /*!
         * \brief Reads compressed header from stream.
         *
         * \param ss    String stream entity to read from.
         *
         * \return  Decompressor instance.
         */
        virtual Decompressor& operator << (std::stringstream &ss);

        /*!
         * \brief Decompression errors are signalled via this.
         */
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
        const diversity::Property<u32_t, Decompressor>  Priority;
        const diversity::Property<pf_t, Decompressor>   Probability;

    protected:
        CSRCDecompressionTable_t probabilityTable;
    }; /* Decompressor */

    /*!
     * \brief Reads compressed header from stream.
     *
     * \param d     Decompressor instance.
     * \param ss    String stream entity to read from.
     *
     * \return  Decompressor instance.
     */
    Decompressor& operator >> (Decompressor &d, std::stringstream &ss);
}; /* fbcd */


#endif /* __DECOMPRESSOR_HPP__ */
