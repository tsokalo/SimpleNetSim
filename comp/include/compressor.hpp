#ifndef __COMPRESSOR_HPP__
#define __COMPRESSOR_HPP__


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
     * \brief Compression context for brr-pkt-header.
     */
    class Compressor
    {
    public:
        /*!
         * \brief ctor.
         *
         * Set configuration here.
         *
         * \param optimisticCnt             Number of times to repeat an update.
         * \param prioritysensitivity       Threshhold for priority update.
         * \param probabilitySensitivity    Threshhold for probability update.
         */
        Compressor(u8_t     optimisticCnt           = 0U,
                   u16_t    prioritySensitivity     = 0U,
                   double   probabilitySensitivity  = 0.0);

        Compressor(const Compressor&) = delete;

        Compressor& operator = (const Compressor&) = delete;

        /*!
         * \brief dtor.
         */
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

        /*!
         * \brief Updates compressor context with new values.
         *
         * Call before operator << or >>.
         *
         * \param priority      Next priority value.
         * \param probabilities Next probability set.
         */
        virtual void Update(u32_t priority, pf_t probabilities);

         /*!
         * \brief Writes compressed header to stream.
         *
         * Call after Update.
         *
         * \param ss    String stream entity to write to.
         *
         * \return  Same as param.
         */
        virtual std::stringstream& operator >> (std::stringstream &ss);

        /*!
         * \brief Returns current metric.
         *
         * \return  Compression ratio.
         */
        virtual float GetCompressionRate() const;

        /*!
         * \brief Compression errors are signalled via this.
         */
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

        const u16_t     prioritySensitivity;
        const double    probabilitySensitivity;

        u32_t   sn;
        size_t  ucBytes;
        size_t  coBytes;

        u32_t   priorityLast;   //!< Last transmitted priority.
        u32_t   priorityPrev;
        u32_t   priorityCurr;

        pf_t probabilitiesLast;
        CSRCCompressionTable_t probabilityTable;
    }; /* Compressor */

    /*!
     * \brief Writes compressed header to stream.
     *
     * Call after Update.
     *
     * \param ss    String stream entity to write to.
     * \param c     Compressor instance..
     *
     * \return  Same as ss.
     */
    std::stringstream& operator << (std::stringstream &ss, Compressor &c);
}; /* fbcd */


#endif /* __COMPRESSOR_HPP__ */
