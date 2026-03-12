#include <iostream>

// void TestGacha();
void TestPlayer();
void TestPlayerGacha();
void TestPity();
void TestCurrencyAtomic();
void TestPitySystem();

int main()
{
    std::cout<<"Running tests..."<<std::endl;

    // TestGacha();
    // TestPlayer();
    // TestPlayerGacha();
    // TestPity();
    TestCurrencyAtomic();
    TestPitySystem();

    std::cout<<"Tests finished"<<std::endl;

    return 0;
}