#pragma once
#include "login.pb.h"
#include "gacha.pb.h"
#include "heartbeat.pb.h"

class ProtocolRegistry
{
public:
    static void RegisterAll();
};