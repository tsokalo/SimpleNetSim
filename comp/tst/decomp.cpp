#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>


#include "compressor.hpp"
#include "decompressor.hpp"


extern size_t g_prioritiesLen;
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
        for (int i = 0U; i < g_prioritiesLen; ++i)
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

TEST(Decompressor, PriorityWithSensitivity)
{
    const u16_t sensitivity = 1000U;
    fbcd::Compressor c { 0U, sensitivity};
    fbcd::Decompressor d;

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            std::stringstream ss;
            pf_t dummy;
            c.Update(g_priorities[i], dummy);
            ss << c;
            u8_t c = 0U;
            d << ss;
            EXPECT_TRUE((d.Priority > (g_priorities[i] - sensitivity))
                || (d.Priority <= (g_priorities[i] + sensitivity)));
            u16_t error =  d.Priority > g_priorities[i]
                ? d.Priority - g_priorities[i]
                : g_priorities[i] - d.Priority;
            //printf("e = %d (%d vs %d)\n", error, d.Priority, g_priorities[i]);
        }
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* PriorityWithSensitivity */

TEST(Decompressor, Probability)
{
    fbcd::Compressor c { 1U, 1000U };
    fbcd::Decompressor d;
    size_t offset = 0U;

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
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
    });
} /* Probability */

TEST(Decompressor, ProbabilityWithSensitivity)
{
    const double sensitivity = 0.2;
    fbcd::Compressor c { 1U, 1000U, sensitivity };
    fbcd::Decompressor d;
    size_t offset = 0U;

    /*EXPECT_NO_THROW
    ({*/
        for (int i = 0U; i < g_prioritiesLen; ++i)
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
                //printf("%f > %f - %f\n", (*i).second, p.second, sensitivity);
                EXPECT_TRUE(((*i).second > (p.second - sensitivity))
                    || ((*i).second <= (p.second + sensitivity)));
            }
        }
    //});

    printf("Savings: %f\n", c.GetCompressionRate());
} /* ProbabilityWithSensitivity */
