#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>


#include "compressor.hpp"


extern size_t g_priorituesLen;
extern unsigned g_priorities[];

TEST(Compressor, PriorityTooLarge)
{
    fbcd::Compressor c;

    const size_t BUF_SZ = 500;
    u8_t buf[BUF_SZ] = { 0U };
    size_t bufSz = BUF_SZ;

    EXPECT_THROW
    ({
        c.Update(static_cast<u32_t>(-1));
    }, fbcd::Compressor::CompressionError);
} /* PriorityTooLarge */

TEST(Compressor, Initialisation)
{
    fbcd::Compressor c;

    EXPECT_NO_THROW
    ({
        c.Update(0x7fffffff);

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
        EXPECT_EQ(bufSz - 1, sizeof (u32_t) + 2);

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
        for (int i = 0U; i < g_priorituesLen; ++i)
        {
            //printf("%u - %0x\n", i, g_priorities[i]);
            c.Update(g_priorities[i]);
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
        for (int i = 0U; i < g_priorituesLen; ++i)
        {
            //printf("%u - %0x\n", i, g_priorities[i]);
            c.Update(g_priorities[i]);
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
        for (int i = 0U; i < g_priorituesLen; ++i)
        {
            c.Update(g_priorities[i]);
            std::stringstream ss;
            ss << c;
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* Optimistic */

