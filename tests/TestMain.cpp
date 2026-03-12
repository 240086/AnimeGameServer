#include <iostream>

void TestGacha();
void TestPlayer();
void TestPlayerGacha();

int main()
{
    std::cout<<"Running tests..."<<std::endl;

    TestGacha();
    TestPlayer();
    TestPlayerGacha();

    std::cout<<"Tests finished"<<std::endl;

    return 0;
}