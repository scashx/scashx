// Copyright (c) 2023 The Bitcoin Core developers
// Copyright (c) 2024 The ScashX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/chaintype.h>

#include <cassert>
#include <optional>
#include <string>

std::string ChainTypeToString(ChainType chain)
{
    switch (chain) {
    case ChainType::MAIN:
        return "main";
    case ChainType::TESTNET:
        return "test";
    case ChainType::SIGNET:
        return "signet";
    case ChainType::REGTEST:
        return "regtest";
    // !SCASHX
    case ChainType::SCASHXMAIN:
        return "scashx";
    case ChainType::SCASHXTESTNET:
        return "scashxtestnet";
    case ChainType::SCASHXREGTEST:
        return "scashxregtest";
    // !SCASHX END
    }
    assert(false);
}

std::optional<ChainType> ChainTypeFromString(std::string_view chain)
{
    if (chain == "main") {
        return ChainType::MAIN;
    } else if (chain == "test") {
        return ChainType::TESTNET;
    } else if (chain == "signet") {
        return ChainType::SIGNET;
    } else if (chain == "regtest") {
        return ChainType::REGTEST;
    // !SCASHX
    } else if (chain == "scashx") {
        return ChainType::SCASHXMAIN;
    } else if (chain == "scashxtestnet") {
        return ChainType::SCASHXTESTNET;
    } else if (chain == "scashxregtest") {
        return ChainType::SCASHXREGTEST;
    // !SCASHX END
    } else {
        return std::nullopt;
    }
}
