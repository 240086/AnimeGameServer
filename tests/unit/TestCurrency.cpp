#include <gtest/gtest.h>
#include "game/player/Currency.h"

TEST(CurrencyTest, Add)
{
    Currency c;

    c.Add(100);

    EXPECT_EQ(c.Get(), 100);
}

TEST(CurrencyTest, SpendSuccess)
{
    Currency c;

    c.Add(100);

    bool ok = c.Spend(60);

    EXPECT_TRUE(ok);
    EXPECT_EQ(c.Get(), 40);
}

TEST(CurrencyTest, SpendFail)
{
    Currency c;

    c.Add(10);

    bool ok = c.Spend(20);

    EXPECT_FALSE(ok);
}