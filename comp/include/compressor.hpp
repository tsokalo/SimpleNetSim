#ifndef __COMPRESSOR_HPP__
#define __COMPRESSOR_HPP__


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
    class Compressor
    {
    public:
        //! \todo threshold
        //! \todo
        //! \todo DELIMITER
        Compressor(u8_t optimisticCnt = 0U);

        Compressor(const Compressor&) = delete;

        Compressor& operator = (const Compressor&) = delete;

        virtual ~Compressor();

        /*!
         * \brief Compress fields.
         *
         * \param priority      Priority field value.
         * \param pBufOut       Output buffer. Not nullptr.
         * \param pBufLenInOut  Output buffer length. Supply total length of the buffer. Will contain the length of the output in bytes.
         *
         * \return Length of the compressed fields in bits.
         */

#if 0
    ss << addr << DELIMITER;
		ss << p.val() << DELIMITER;
		ss << (uint16_t)pf.size() << DELIMITER;
		for(auto p : pf)ss << p.first << DELIMITER << p.second << DELIMITER;
#endif
        virtual void Update(u32_t priority);

        virtual std::stringstream& operator >> (std::stringstream &ss);

        virtual float GetCompressionRate() const;

        class CompressionError : public std::exception
        {
        public:
            CompressionError(const char* msg);

            virtual ~CompressionError();

            virtual const char* what() const throw();

        protected:
            std::string msg;
        }; /* CompressionError */

    protected:
        const u8_t  optimisticCnt;
        bool        isOptimistic;
        u8_t        optimisticTo;

        u32_t   sn;
        size_t  ucBytes;
        size_t  coBytes;

        u32_t   priorityPrev;
        u32_t   priorityCurr;
    }; /* Compressor */

    std::stringstream& operator << (std::stringstream &ss, Compressor &c);
}; /* fbcd */


#endif /* __COMPRESSOR_HPP__ */
