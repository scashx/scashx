#include <iostream>
#include "crypto/sha256.h"
#include "primitives/block.h"
#include "util/strencodings.h"
#include "consensus/merkle.h"
#include "arith_uint256.h"
#include "consensus/merkle.h"
#include "bignumber.h"
/* build command
    g++ -m64 -std=c++20 -o GenerateGenesisBlock GenerateGenesisBlock.cpp  -Wl,-z,relro -Wl,-z,now -Wl,-z,separate-code -pie    -static -I. -I../src/config  -I/work/scashx/scashx/depends/x86_64-pc-linux-gnu/include/ -L/work/scashx/scashx/depends/x86_64-pc-linux-gnu/lib -lbitcoinconsensus
*/

uint256 ComputeGenesisHash(CBlock genesis) {
    return genesis.GetHash();
}

static CBlock CreateGenesisBlock1(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock1(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

int main() {
    uint32_t nTime = 1707123456;  // Set timestamp
    //uint32_t nBits = 0x1d00ffff;  // Set difficulty
    uint32_t nBits = 0x7fffffff;  // Set difficulty
    uint32_t nNonce = 0;          // Start nonce from 0

    CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, 1, 50 * COIN);

    while (true) {
        genesis.nNonce = nNonce;
        uint256 hash = ComputeGenesisHash(genesis);
        //arith_uint256 bnNew;
        //bnNew = arith_uint256().SetCompact(nBits);
        if (UintToArith256(hash) < UintToArith256(CBigNum().SetCompact(nBits).getuint256())) {

        //if (UintToArith256(hash) < bnNew) {
            std::cout << "Genesis block found!" << std::endl;
            std::cout << "Nonce: " << nNonce << std::endl;
            std::cout << "Genesis Hash: " << hash.ToString() << std::endl;
            std::cout << "Merkle Root: " << genesis.hashMerkleRoot.ToString() << std::endl;
            break;
        }else{
            if (nNonce % 1000000 == 0) std::cout << "hash " << hash.ToString() << " bnNew " << CBigNum().SetCompact(nBits).getuint256().ToString() << std::endl;
        }
        nNonce++;
        if (nNonce % 1000000 == 0) std::cout << "Tried " << nNonce << " hashes..." << std::endl;
    }

    return 0;
}
