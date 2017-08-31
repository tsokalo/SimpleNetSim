#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>


#include "compressor.hpp"
#include "decompressor.hpp"


extern size_t g_priorituesLen;
extern unsigned g_priorities[];
extern size_t LoadProbability(pf_t&, size_t);

TEST(Decompressor, Initialisation)
{
    fbcd::Compressor c;
    fbcd::Decompressor d;
    std::stringstream ss;

    EXPECT_NO_THROW
    ({
        pf_t dummy;
        c.Update(0x7fffffff, dummy);
        ss << c;
        d << ss;
        EXPECT_EQ(d.Priority, 0x7fffffffU);
        EXPECT_EQ((*d.Probability).size(), 0);
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
            pf_t dummy;
            c.Update(g_priorities[i], dummy);
            ss << c;
            u8_t c = 0U;
            d << ss;
            EXPECT_EQ(d.Priority, g_priorities[i]);
        }
    });
} /* Priority */

TEST(Decompressor, Probability)
{
    fbcd::Compressor c { 1U, 1000U };
    fbcd::Decompressor d;
    size_t offset = 0U;

    /*EXPECT_NO_THROW
    ({*/
        for (int i = 0U; i < g_priorituesLen; ++i)
        {
            pf_t probability;
            offset = LoadProbability(probability, offset);

            c.Update(g_priorities[i], probability);
            std::stringstream ss;
            ss << c;

            u8_t c = 0U;
            d << ss;

            EXPECT_EQ((*d.Probability).size(), probability.size());
            for (auto p : probability)
            {
                auto i = (*d.Probability).find(p.first);
                ASSERT_FALSE(i == (*d.Probability).end());
                EXPECT_FLOAT_EQ((*i).second, p.second);
            }
        }
    //});
} /* Probability */
