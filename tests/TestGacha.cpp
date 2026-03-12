#include <iostream>
#include <map>

#include "game/gacha/GachaSystem.h"
#include "game/player/PlayerManager.h"

void TestGacha()
{
    std::map<int,int> count;

    for(int i=0;i<100000;i++)
    {
        auto item = GachaSystem::Instance().DrawOnce();

        count[item.rarity]++;
    }

    std::cout<<"---- Gacha Test Result ----"<<std::endl;

    for(auto& [rarity,c]:count)
    {
        double p = (double)c / 100000.0 * 100;

        std::cout
            << "rarity "
            << rarity
            << " -> "
            << p
            << "%"
            << std::endl;
    }
}

void TestPlayer()
{
    auto player = PlayerManager::Instance().CreatePlayer(1001);

    player->GetInventory().AddItem(1);

    player->GetGachaHistory().Record(4);

    std::cout << "TestPlayer()\n";
}