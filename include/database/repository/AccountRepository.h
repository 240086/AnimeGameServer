#pragma once

#include <string>
#include <optional>

class AccountRepository
{
public:
    static AccountRepository& Instance();

    std::optional<uint64_t> GetAccountId(
        const std::string& username,
        const std::string& password);
};