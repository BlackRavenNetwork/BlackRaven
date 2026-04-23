// One-off helper to mine genesis (KAWPOW) for BlackRaven — build from src/:
// g++ -std=c++17 -DHAVE_CONFIG_H -I. -Iobj -o genesis_finder ../contrib/genesis_finder.cpp \
//     obj/build_info.o primitives/block.o arith_uint256.o uint256.o crypto/*.o algo/*.o \
//     utilstrencodings.o tinyformat.o hash.o consensus/merkle.o primitives/transaction.o \
//     compressor.o script/script.o script/standard.o pubkey.o keystore.o ...

#include <config/raven-config.h>

#include "amount.h"
#include "arith_uint256.h"
#include "consensus/merkle.h"
#include "primitives/block.h"
#include "pow.h"
#include "tinyformat.h"
#include "utilstrencodings.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

static CBlock MakeGenesis(const char* pszTimestamp, const CScript& genesisOutputScript,
    uint32_t nTime, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << CScriptNum(0) << 486604799 << CScriptNum(4)
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp,
            (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = 0;
    genesis.nHeight = 0;
    genesis.nNonce64 = 0;
    genesis.mix_hash.SetNull();
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

int main(int argc, char** argv)
{
    const char* pszTimestamp = argc > 1 ? argv[1] : "BlackRaven BLKR 2026-04-21 fair launch for asset issuance";
    const CScript genesisOutputScript = CScript()
        << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f")
        << OP_CHECKSIG;

    const uint32_t nTime = argc > 2 ? static_cast<uint32_t>(strtoul(argv[2], nullptr, 10)) : 1745251200u;
    const uint32_t nBits = argc > 3 ? static_cast<uint32_t>(strtoul(argv[3], nullptr, 0)) : 0x1f00ffffu;
    const int32_t nVersion = argc > 4 ? atoi(argv[4]) : 4;
    const CAmount genesisReward = 5000 * COIN;

    arith_uint256 target;
    bool fNegative, fOverflow;
    target.SetCompact(nBits, &fNegative, &fOverflow);
    assert(!fNegative && !fOverflow);

    // Force KAWPOW hashing path in block header serialization/hash.
    nKAWPOWActivationTime = 0;

    std::cout << "Mining genesis KAWPOW; target " << target.GetHex() << std::endl;

    for (uint64_t n = 0; n < 500000000ULL; n++) {
        CBlock genesis = MakeGenesis(pszTimestamp, genesisOutputScript, nTime, nBits, nVersion, genesisReward);
        genesis.nNonce64 = n;
        uint256 mixHash;
        uint256 h = genesis.GetHashFull(mixHash);
        if (UintToArith256(h) <= target) {
            std::cout << "FOUND nonce64=" << n << std::endl;
            std::cout << "hashGenesisBlock=" << h.GetHex() << std::endl;
            std::cout << "mixHash=" << mixHash.GetHex() << std::endl;
            std::cout << "hashMerkleRoot=" << genesis.hashMerkleRoot.GetHex() << std::endl;
            return 0;
        }
        if ((n & 0xfffff) == 0)
            std::cout << "tried nonce64 up to " << n << "\r" << std::flush;
    }
    std::cout << "not found in range\n";
    return 1;
}
