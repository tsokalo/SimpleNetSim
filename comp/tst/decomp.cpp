#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>


#include "compressor.hpp"
#include "decompressor.hpp"


extern size_t g_priorituesLen;
extern unsigned g_priorities[];

TEST(Decompressor, Initialisation)
{
    fbcd::Compressor c;
    fbcd::Decompressor d;
    std::stringstream ss;

    EXPECT_NO_THROW
    ({
        c.Update(0x7fffffff);
        ss << c;
        d << ss;
        EXPECT_EQ(d.Priority, 0x7fffffffU);
    });
} /* Initialisation */

TEST(Decompressor, Priority)
{
    fbcd::Compressor c;
    fbcd::Decompressor d;

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_priorituesLen; ++i)
        {
            std::stringstream ss;
            c.Update(g_priorities[i]);
            ss << c;
            u8_t c = 0U;
            d << ss;
            EXPECT_EQ(d.Priority, g_priorities[i]);
        }
    });
} /* Priority */
