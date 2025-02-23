#include <iostream>
#include "crypto/sha256.h"
#include "primitives/block.h"
#include "util/strencodings.h"
#include "consensus/merkle.h"
#include "arith_uint256.h"
#include "consensus/amount.h"
#include "consensus/merkle.h"
#include "hash.h"
#include "pow.h"
#include "randomx.h"


/* build command
    g++ -m64 -std=c++20 -o GenerateGenesisBlock GenerateGenesisBlock.cpp  -Wl,-z,relro -Wl,-z,now -Wl,-z,separate-code -pie    -static -I. -I../src/config  -I/work/scashx/scashx/depends/x86_64-pc-linux-gnu/include/ -L/work/scashx/scashx/depends/x86_64-pc-linux-gnu/lib -lbitcoinconsensus -lrandomx
*/

// Compute block hash over entire header (including RandomX fields) even if global flag is not set
static uint256 GetHashOfScashXGenesisBlock(const CBlock& genesis) {
    CBlockHeader rx_blockHeader(genesis);
    unsigned char hash[32];
    CHash256().Write({(unsigned char *)&rx_blockHeader, sizeof(rx_blockHeader)}).Finalize(hash);
    return uint256(hash);
}


// Compute randomx commitment from block header. If inHash parameter is not provided, use hash from block header.
static uint256 GetRandomXCommitment_(const CBlockHeader& block) {
    uint256 rx_hash = block.hashRandomX;
    CBlockHeader rx_blockHeader(block);
    rx_blockHeader.hashRandomX.SetNull();   // set to null when hashing
    char rx_cm[RANDOMX_HASH_SIZE];
    randomx_calculate_commitment(&rx_blockHeader, sizeof(rx_blockHeader), rx_hash.data(), rx_cm);
    return uint256(std::vector<unsigned char>(rx_cm, rx_cm + sizeof(rx_cm)));
}


static CBlock CreateGenesisBlock_(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
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
    const char* pszTimestamp = "22/Feb/2024 S&P 5087.03 @elonmusk 1760819426688115087 Congrats";//"The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock_(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

int main() {
    uint32_t nTime = 1708650456;//1231006505;//1707123456;  // Set timestamp
    uint32_t nBits = 0x1e0fffff;//0x1d00ffff;//0x20dfffff;  // Set difficulty
    uint32_t nNonce = 981715;//20076863;//2083236890;//0;          // Start nonce from 0

    bool fNegative = true;
    bool fOverflow = true;

    CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, 1, 50 * COIN);
	genesis.hashRandomX = uint256S("33c450e0152826e3a8948b01464cf9182344a1544b3ddcf6153dd04b62938d01");

    while (true) {
        genesis.nNonce = nNonce;
        uint256 hash = GetHashOfScashXGenesisBlock(genesis);//genesis.GetHash();
        arith_uint256 bnNew;
        bnNew = arith_uint256().SetCompact(nBits, &fNegative, &fOverflow);
        //if (UintToArith256(hash) < UintToArith256(CBigNum().SetCompact(nBits).getuint256())) {

		//std::cout << "Genesis Hash: " << UintToArith256(hash).ToString() << std::endl;
		
        if (UintToArith256(GetRandomXCommitment_(genesis)) < bnNew) {
            std::cout << "Genesis block found!" << std::endl;
            std::cout << "Nonce: " << nNonce << std::endl;
			std::cout << "limit: " << bnNew.ToString() << std::endl;
            std::cout << "Genesis Hash: " << hash.ToString() << std::endl;
			std::cout << "Genesis XCommitment Hash: " << GetRandomXCommitment_(genesis).ToString() << std::endl;
            std::cout << "Merkle Root: " << genesis.hashMerkleRoot.ToString() << std::endl;
            break;
        }/*else{
            if (nNonce % 1000000 == 0) std::cout << "hash " << hash.ToString() << " bnNew " << bnNew.ToString() << std::endl;
        }*/
        nNonce++;
        if (nNonce % 1000000 == 0) std::cout << "Tried " << nNonce << " hashes..." << std::endl;
    }

    return 0;
}
