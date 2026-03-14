// F:\VSCode_project\Cpp_Proj\AnimeGameServer\include\services\GachaService.h
#pragma once

#include "services/BaseService.h"
#include "network/Connection.h"

struct GachaResponse
{
    uint32_t itemId;
    uint8_t rarity;
};
class GachaService : public BaseService
{
public:

    static GachaService& Instance();

    void Init() override;

    void HandleGacha(Connection* conn, const char* data, size_t len);
    void HandleGachaTen(Connection* conn, const char* data, size_t len);
};