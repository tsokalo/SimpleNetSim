#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>


#include "compressor.hpp"


extern size_t g_prioritiesLen;
extern unsigned g_priorities[];

extern double g_pf[];
extern size_t LoadProbability(pf_t&, size_t, double* = nullptr);

TEST(Compressor, PriorityTooLarge)
{
    fbcd::Compressor c;

    const size_t BUF_SZ = 500;
    u8_t buf[BUF_SZ] = { 0U };
    size_t bufSz = BUF_SZ;

    EXPECT_THROW
    ({
        pf_t dummy;
        c.Update(static_cast<u32_t>(-1), dummy);
    }, fbcd::Compressor::CompressionError);
} /* PriorityTooLarge */

TEST(Compressor, Initialisation)
{
    fbcd::Compressor c;

    EXPECT_NO_THROW
    ({
        pf_t dummy;
        c.Update(0x7fffffff, dummy);

        std::stringstream ss;
        ss << c;

        const size_t BUF_SZ = 500;
        u8_t buf[BUF_SZ] = { 0U };

        size_t bufSz = 0U;
        while (!ss.eof())
        {
            u8_t tmp = 0U;
            ss >> tmp;
            //printf("%c", tmp);
            buf[bufSz] = tmp;
            ++bufSz;
        }
        //printf("\n");
        EXPECT_EQ(bufSz - 1, sizeof (u32_t) + 2 + 1);

        u8_t bufTest[500] = { 0x80 };
        bufTest[1] = 0x0f;
        bufTest[2] = 0x7f;
        bufTest[3] = 0xff;
        bufTest[4] = 0xff;
        bufTest[5] = 0xff;
        for (size_t i = 0U; i < bufSz - 1; ++i)
        {
            EXPECT_EQ(buf[i], bufTest[i]);
        }

        //writePacketDump(bufTest, bufSz);
        //writePacketDump(buf, bufSz);
    });
} /* Initialisation */

TEST(Compressor, Priority)
{
    fbcd::Compressor c;

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            //printf("%u - %0x\n", i, g_priorities[i]);
            pf_t dummy;
            c.Update(g_priorities[i], dummy);
            std::stringstream ss;
            ss << c;
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* Priority */

TEST(Compressor, Optimistic)
{
    fbcd::Compressor c { 1U };

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            //printf("%u - %0x\n", i, g_priorities[i]);
            pf_t dummy;
            c.Update(g_priorities[i], dummy);
            std::stringstream ss;
            ss << c;
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* Optimistic */

TEST(Compressor, PrioritySensitivity)
{
    fbcd::Compressor c { 1U, 1000U };

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            pf_t dummy;
            c.Update(g_priorities[i], dummy);
            std::stringstream ss;
            ss << c;
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* PrioritySensitivity */

TEST(Compressor, Probability)
{
    fbcd::Compressor c { 1U, 1000U };
    size_t offset = 0U;

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            pf_t probability;
            offset = LoadProbability(probability, offset, (double*)g_pf);

            c.Update(g_priorities[i], probability);
            std::stringstream ss;
            ss << c;
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* Probability */
