// F:\VSCode_project\Cpp_Proj\AnimeGameServer\tests\unit\TestSession.cpp
#include <gtest/gtest.h>

#include "network/session/SessionManager.h"
#include "game/player/PlayerManager.h"

TEST(SessionTest, BindPlayer)
{
    auto session = SessionManager::Instance().CreateSession();

    auto player = PlayerManager::Instance().CreatePlayer(1001);

    session->BindPlayer(player);

    ASSERT_NE(session->GetPlayer(), nullptr);
    EXPECT_EQ(session->GetPlayer()->GetId(), 1001);
}