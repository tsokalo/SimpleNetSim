#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <cstdio>
#include <cstring>


#include "compressor.hpp"
#include "decompressor.hpp"


extern size_t g_prioritiesLen;
extern u32_t g_priorities[];

extern size_t g_logPrioritiesNode0Len;
extern u32_t g_logPrioritiesNode0[];
extern size_t g_logPfNode0Len;
extern u32_t g_logPfNode0[];
extern size_t g_logPrioritiesNode1Len;
extern u32_t g_logPrioritiesNode1[];
extern size_t g_logPfNode1Len;
extern u32_t g_logPfNode1[];
extern size_t g_logPrioritiesNode2Len;
extern u32_t g_logPrioritiesNode2[];
extern size_t g_logPfNode2Len;
extern u32_t g_logPfNode2[];
extern size_t g_logPrioritiesNode3Len;
extern u32_t g_logPrioritiesNode3[];

extern size_t g_logTop3_1_PfNode0Len;
extern double g_logTop3_1_PfNode0[];
extern size_t g_logTop3_1_PfNode1Len;
extern double g_logTop3_1_PfNode1[];
extern size_t g_logTop3_1_PfNode2Len;
extern double g_logTop3_1_PfNode2[];
extern size_t g_logTop3_1_PfNode3Len;
extern double g_logTop3_1_PfNode3[];

extern double g_pf[];
extern size_t LoadProbability(pf_t&, size_t, double* = nullptr);

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

TEST(Decompressor, LogPriorityNode0WithSensitivity)
{
    std::fstream csv { "log/priorityNode0.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "priorityError"
        << std::endl;

    const size_t sensitivityMin = 0U;
    const u16_t sensitivityMax = 30000U;
    const u16_t sensitivityStep = 1000U;
    for (u16_t sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError   = 0.0f;
        float compressionRatio          = 0.0f;

        EXPECT_NO_THROW
        ({

            size_t i = 0U;
            for (i = sensitivityMin; i < g_logPrioritiesNode0Len; ++i)
            {
                std::stringstream ss;
                pf_t dummy;
                c.Update(g_logPrioritiesNode0[i], dummy);
                ss << c;
                u8_t c = 0U;
                d << ss;
                EXPECT_TRUE((d.Priority > (g_logPrioritiesNode0[i] - sensitivity))
                    || (d.Priority <= (g_logPrioritiesNode0[i] + sensitivity)));
                u16_t error =  d.Priority > g_logPrioritiesNode0[i]
                    ? d.Priority - g_logPrioritiesNode0[i]
                    : g_logPrioritiesNode0[i] - d.Priority;
                //printf("e = %d (%d vs %d)\n", error, d.Priority, g_logPrioritiesNode0[i]);
                totalError += error / (float)g_logPrioritiesNode0[i];
            }


            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
            //printf("Savings (sensitivity = %d): %f\n", sensitivity, c.GetCompressionRate());
        });
    }
} /* LogPriorityNode0WithSensitivity */

TEST(Decompressor, LogPriorityNode1WithSensitivity)
{
    std::fstream csv { "log/priorityNode1.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "priorityError"
        << std::endl;

    const size_t sensitivityMin = 0U;
    const u16_t sensitivityMax = 30000U;
    const u16_t sensitivityStep = 1000U;
    for (u16_t sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError   = 0.0f;
        float compressionRatio          = 0.0f;

        EXPECT_NO_THROW
        ({

            size_t i = 0U;
            for (i = sensitivityMin; i < g_logPrioritiesNode1Len; ++i)
            {
                std::stringstream ss;
                pf_t dummy;
                c.Update(g_logPrioritiesNode1[i], dummy);
                ss << c;
                u8_t c = 0U;
                d << ss;
                EXPECT_TRUE((d.Priority > (g_logPrioritiesNode1[i] - sensitivity))
                    || (d.Priority <= (g_logPrioritiesNode1[i] + sensitivity)));
                u16_t error =  d.Priority > g_logPrioritiesNode1[i]
                    ? d.Priority - g_logPrioritiesNode1[i]
                    : g_logPrioritiesNode1[i] - d.Priority;
                //printf("e = %d (%d vs %d)\n", error, d.Priority, g_logPrioritiesNode1[i]);
                totalError += error / (float)g_logPrioritiesNode1[i];
            }


            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
            //printf("Savings (sensitivity = %d): %f\n", sensitivity, c.GetCompressionRate());
        });
    }
} /* LogPriorityNode1WithSensitivity */

TEST(Decompressor, LogPriorityNode2WithSensitivity)
{
    std::fstream csv { "log/priorityNode2.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "priorityError"
        << std::endl;

    const size_t sensitivityMin = 0U;
    const u16_t sensitivityMax = 30000U;
    const u16_t sensitivityStep = 1000U;
    for (u16_t sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError   = 0.0f;
        float compressionRatio          = 0.0f;

        EXPECT_NO_THROW
        ({

            size_t i = 0U;
            for (i = sensitivityMin; i < g_logPrioritiesNode2Len; ++i)
            {
                std::stringstream ss;
                pf_t dummy;
                c.Update(g_logPrioritiesNode2[i], dummy);
                ss << c;
                u8_t c = 0U;
                d << ss;
                EXPECT_TRUE((d.Priority > (g_logPrioritiesNode2[i] - sensitivity))
                    || (d.Priority <= (g_logPrioritiesNode2[i] + sensitivity)));
                u16_t error =  d.Priority > g_logPrioritiesNode2[i]
                    ? d.Priority - g_logPrioritiesNode2[i]
                    : g_logPrioritiesNode2[i] - d.Priority;
                //printf("e = %d (%d vs %d)\n", error, d.Priority, g_logPrioritiesNode2[i]);
                totalError += error / (float)g_logPrioritiesNode2[i];
            }


            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
            //printf("Savings (sensitivity = %d): %f\n", sensitivity, c.GetCompressionRate());
        });
    }
} /* LogPriorityNode2WithSensitivity */

TEST(Decompressor, LogPriorityNode3WithSensitivity)
{
    std::fstream csv { "log/priorityNode3.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "priorityError"
        << std::endl;

    const size_t sensitivityMin = 0U;
    const u16_t sensitivityMax = 30000U;
    const u16_t sensitivityStep = 1000U;
    for (u16_t sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError   = 0.0f;
        float compressionRatio          = 0.0f;

        EXPECT_NO_THROW
        ({

            size_t i = 0U;
            for (i = sensitivityMin; i < g_logPrioritiesNode3Len; ++i)
            {
                std::stringstream ss;
                pf_t dummy;
                c.Update(g_logPrioritiesNode3[i], dummy);
                ss << c;
                u8_t c = 0U;
                d << ss;
                EXPECT_TRUE((d.Priority > (g_logPrioritiesNode3[i] - sensitivity))
                    || (d.Priority <= (g_logPrioritiesNode3[i] + sensitivity)));
                u16_t error =  d.Priority > g_logPrioritiesNode3[i]
                    ? d.Priority - g_logPrioritiesNode3[i]
                    : g_logPrioritiesNode3[i] - d.Priority;
                //printf("e = %d (%d vs %d)\n", error, d.Priority, g_logPrioritiesNode3[i]);
                totalError += error / (float)g_logPrioritiesNode3[i];
            }


            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
            //printf("Savings (sensitivity = %d): %f\n", sensitivity, c.GetCompressionRate());
        });
    }
} /* LogPriorityNode3WithSensitivity */

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
            offset = LoadProbability(probability, offset, (double*)g_pf);

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

    EXPECT_NO_THROW
    ({
        for (int i = 0U; i < g_prioritiesLen; ++i)
        {
            pf_t probability;
            offset = LoadProbability(probability, offset, (double*)g_pf);

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
    });

    printf("Savings: %f\n", c.GetCompressionRate());
} /* ProbabilityWithSensitivity */

TEST(Decompressor, ProbabilityNode0WithSensitivity)
{
    std::fstream csv { "log/pfNode0.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "pfError"
        << std::endl;

    const double sensitivityMin = 0.0;
    const double sensitivityMax = 0.5;
    const double sensitivityStep = 0.01;

    for (double sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError        = 0.0f;
        float compressionRatio  = 0.0f;
        size_t offset = 0U;

        /*EXPECT_NO_THROW
        ({*/
            size_t i = 0U;
            for (i = sensitivityMin; i < g_logTop3_1_PfNode0Len; ++i)
            {
                pf_t probability;
                offset = LoadProbability(probability, offset, (double*)g_logTop3_1_PfNode0);
                //printf("%f\n", probability[0]);
                //printf("%d\n", offset);

                c.Update(0U, probability);
                std::stringstream ss;
                ss << c;

                u8_t c = 0U;
                d << ss;

                float error =  0.0;
                EXPECT_EQ((*d.Probability).size(), probability.size());
                for (auto p : probability)
                {
                    auto y = (*d.Probability).find(p.first);
                    ASSERT_FALSE(y == (*d.Probability).end());
                    //printf("%f > %f - %f\n", (*i).second, p.second, sensitivity);
                    EXPECT_TRUE(((*y).second > (p.second - sensitivity))
                        || ((*y).second <= (p.second + sensitivity)));
                    error += (*y).second > p.second
                        ? (*y).second - p.second
                        : p.second - (*y).second;
                    //printf("%f (%f %f)\n", error, (*y).second, p.second);
                }

                if (probability.size())
                {
                    totalError += error / probability.size();  // / (float)g_logPrioritiesNode3[i];
                }
            }

            float averageError = totalError / (i - sensitivityMin);
            //printf("%f = %f / %f\n", averageError, totalError, i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
        //});
    }
} /* ProbabilityNode0WithSensitivity */

TEST(Decompressor, ProbabilityNode1WithSensitivity)
{
    std::fstream csv { "log/pfNode1.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "pfError"
        << std::endl;

    const double sensitivityMin = 0.0;
    const double sensitivityMax = 0.5;
    const double sensitivityStep = 0.01;

    for (double sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError        = 0.0f;
        float compressionRatio  = 0.0f;
        size_t offset = 0U;

        EXPECT_NO_THROW
        ({
            size_t i = 0U;
            for (i = sensitivityMin; i < g_logTop3_1_PfNode1Len; ++i)
            {
                pf_t probability;
                offset = LoadProbability(probability, offset, (double*)g_logTop3_1_PfNode1);

                c.Update(0U, probability);
                std::stringstream ss;
                ss << c;

                u8_t c = 0U;
                d << ss;

                float error =  0.0;
                EXPECT_EQ((*d.Probability).size(), probability.size());
                for (auto p : probability)
                {
                    auto i = (*d.Probability).find(p.first);
                    ASSERT_FALSE(i == (*d.Probability).end());
                    //printf("%f > %f - %f\n", (*i).second, p.second, sensitivity);
                    EXPECT_TRUE(((*i).second > (p.second - sensitivity))
                        || ((*i).second <= (p.second + sensitivity)));
                    error += (*i).second > p.second
                        ? (*i).second - p.second
                        : p.second - (*i).second;
                }

                if (probability.size())
                {
                    totalError += error / probability.size();  // / (float)g_logPrioritiesNode3[i];
                }
            }

            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
        });
    }
} /* ProbabilityNode1WithSensitivity */

TEST(Decompressor, ProbabilityNode2WithSensitivity)
{
    std::fstream csv { "log/pfNode2.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "pfError"
        << std::endl;

    const double sensitivityMin = 0.0;
    const double sensitivityMax = 0.5;
    const double sensitivityStep = 0.01;

    for (double sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError        = 0.0f;
        float compressionRatio  = 0.0f;
        size_t offset = 0U;

        EXPECT_NO_THROW
        ({
            size_t i = 0U;
            for (i = sensitivityMin; i < g_logTop3_1_PfNode2Len; ++i)
            {
                pf_t probability;
                offset = LoadProbability(probability, offset, (double*)g_logTop3_1_PfNode2);

                c.Update(0U, probability);
                std::stringstream ss;
                ss << c;

                u8_t c = 0U;
                d << ss;

                float error =  0.0;
                EXPECT_EQ((*d.Probability).size(), probability.size());
                for (auto p : probability)
                {
                    auto i = (*d.Probability).find(p.first);
                    ASSERT_FALSE(i == (*d.Probability).end());
                    //printf("%f > %f - %f\n", (*i).second, p.second, sensitivity);
                    EXPECT_TRUE(((*i).second > (p.second - sensitivity))
                        || ((*i).second <= (p.second + sensitivity)));
                    error += (*i).second > p.second
                        ? (*i).second - p.second
                        : p.second - (*i).second;
                }

                if (probability.size())
                {
                    totalError += error / probability.size();  // / (float)g_logPrioritiesNode3[i];
                }
            }

            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
        });
    }
} /* ProbabilityNode2WithSensitivity */

TEST(Decompressor, ProbabilityNode3WithSensitivity)
{
    std::fstream csv { "log/pfNode3.csv", std::fstream::out };
    csv << "sensitivity"
        << ","
        << "compressionRatio"
        << ','
        << "pfError"
        << std::endl;

    const double sensitivityMin = 0.0;
    const double sensitivityMax = 0.5;
    const double sensitivityStep = 0.01;

    for (double sensitivity = 0U;
        sensitivity < sensitivityMax;
        sensitivity += sensitivityStep)
    {
        fbcd::Compressor c { 0U, 0U, sensitivity};
        fbcd::Decompressor d;
        float totalError        = 0.0f;
        float compressionRatio  = 0.0f;
        size_t offset = 0U;

        EXPECT_NO_THROW
        ({
            size_t i = 0U;
            for (i = sensitivityMin; i < g_logTop3_1_PfNode3Len; ++i)
            {
                pf_t probability;
                offset = LoadProbability(probability, offset, (double*)g_logTop3_1_PfNode3);

                c.Update(0U, probability);
                std::stringstream ss;
                ss << c;

                u8_t c = 0U;
                d << ss;

                float error =  0.0;
                EXPECT_EQ((*d.Probability).size(), probability.size());
                for (auto p : probability)
                {
                    auto i = (*d.Probability).find(p.first);
                    ASSERT_FALSE(i == (*d.Probability).end());
                    //printf("%f > %f - %f\n", (*i).second, p.second, sensitivity);
                    EXPECT_TRUE(((*i).second > (p.second - sensitivity))
                        || ((*i).second <= (p.second + sensitivity)));
                    error += (*i).second > p.second
                        ? (*i).second - p.second
                        : p.second - (*i).second;
                }

                if (probability.size())
                {
                    totalError += error / probability.size();  // / (float)g_logPrioritiesNode3[i];
                }
            }

            float averageError = totalError / (i - sensitivityMin);
            csv << sensitivity << "," << c.GetCompressionRate() << "," << averageError << std::endl;
        });
    }
} /* ProbabilityNode3WithSensitivity */
