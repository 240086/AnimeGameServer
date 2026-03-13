// F:\VSCode_project\Cpp_Proj\AnimeGameServer\tests\TestGacha.cpp
#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include "game/gacha/GachaSystem.h"
#include "game/player/PlayerManager.h"
#include "game/player/Currency.h"
#include "game/gacha/GachaPoolManager.h"
#include "common/config/Config.h"
#include "game/player/PlayerLogicLoop.h"

// void TestGacha()
// {
//     std::map<int, int> count;

//     for (int i = 0; i < 100000; i++)
//     {
//         auto item = GachaSystem::Instance().DrawOnce();

//         count[item.rarity]++;
//     }

//     std::cout << "---- Gacha Test Result ----" << std::endl;

//     for (auto &[rarity, c] : count)
//     {
//         double p = (double)c / 100000.0 * 100;

//         std::cout
//             << "rarity "
//             << rarity
//             << " -> "
//             << p
//             << "%"
//             << std::endl;
//     }
// }

void TestPlayer()
{
    auto player = PlayerManager::Instance().CreatePlayer(1001);

    player->GetInventory().AddItem(1);

    player->GetGachaHistory().Record(4);

    std::cout << "TestPlayer()\n";
}

void TestPlayerGacha()
{
    auto player = PlayerManager::Instance().CreatePlayer(1);

    for (int i = 0; i < 10; i++)
    {
        auto item = GachaSystem::Instance().DrawOnce(*player);

        player->GetInventory().AddItem(item.id);

        player->GetGachaHistory().Record(item.rarity);
    }

    auto items = player->GetInventory().GetItems();

    std::cout << "inventory size: " << items.size() << std::endl;
}

void TestPity()
{
    auto player = PlayerManager::Instance().CreatePlayer(1);

    int five = 0;

    for (int i = 0; i < 100000; i++)
    {
        auto item = GachaSystem::Instance().DrawOnce(*player);

        if (item.rarity == 5)
            five++;
    }

    std::cout << "five star count: " << five << std::endl;
}

void TestCurrencyAtomic()
{
    Currency currency;

    const int THREADS = 8;
    const int OPS = 10000;

    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; i++)
    {
        threads.emplace_back([&]()
                             {
            for(int j=0;j<OPS;j++)
            {
                currency.Add(1);
                currency.Spend(1);
            } });
    }

    for (auto &t : threads)
        t.join();

    std::cout << "currency final: "
              << currency.Get()
              << std::endl;
}

void TestPitySystem()
{
    auto player = PlayerManager::Instance().CreatePlayer(1);

    int five = 0;

    for (int i = 0; i < 10000000; i++)
    {
        auto item = GachaSystem::Instance().DrawOnce(*player);

        if (item.rarity == 5)
            five++;
    }

    std::cout << "5-star count: "
              << five
              << std::endl;
}

void TestGachaPool()
{
    std::cout << 1;
    GachaPoolManager::Instance().LoadConfig(R"(F:\VSCode_project\Cpp_Proj\AnimeGameServer\config\gacha_pool.yaml)");
    std::cout << 2;
    auto &pool =
        GachaPoolManager::Instance().GetPool(1);

    for (int i = 0; i < 10; i++)
    {
        auto item = pool.DrawByRarity(5);

        std::cout
            << "draw item "
            << item.name
            << std::endl;
    }
}

void TestGachaSimulation()
{
    std::cout << "=== Gacha Simulation Test ===" << std::endl;

    // load config
    GachaPoolManager::Instance().LoadConfig("gacha_pool.yaml");

    // start logic loop
    PlayerLogicLoop::Instance().Start();

    // create player
    auto player = PlayerManager::Instance().CreatePlayer(1);

    player->GetCurrency().Add(100000);

    const int TEST_DRAWS = 1000;

    for (int i = 0; i < TEST_DRAWS; i++)
    {
        player->GetCommandQueue().Push([player]()
                                       {
            const int COST = 160;

            if(!player->GetCurrency().Spend(COST))
                return;

            auto item =
                GachaSystem::Instance().DrawOnce(*player);

            player->GetInventory().AddItem(item.id);

            player->GetGachaHistory().Record(item.rarity); });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto items = player->GetInventory().GetItems();

    std::cout << "Total items: " << items.size() << std::endl;

    PlayerLogicLoop::Instance().Stop();
}
