#include <iostream>
#include <cstring>
#include "crypto/sha256.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "util/strencodings.h"
#include "consensus/merkle.h"
#include "arith_uint256.h"
#include "consensus/amount.h"
#include "hash.h"
#include "pow.h"
#include "randomx.h"

// Build comaand from src/kernel
// g++ -m64 -std=c++20 -o GenerateGenesisBlock GenerateGenesisBlock.cpp   -Wl,-z,relro -Wl,-z,now -Wl,-z,separate-code -pie -static   -I. -I../ -I../src/config -I/home/boredmember/scashx/depends/x86_64-pc-linux-gnu/include   -L/home/boredmember/scashx/depends/x86_64-pc-linux-gnu/lib -lrandomx -lbitcoinconsensus

// CreateGenesisBlock: builds a block with a coinbase transaction containing the timestamp message.
static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, 
                                 uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, 
                                 const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // Coinbase script: includes a fixed number, a small push, and the timestamp message.
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4)
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp, 
                                      (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
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

// CreateScashXGenesisBlock: helper function that uses a custom timestamp and output script.
static CBlock CreateScashXGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, 
                                       int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "930am NYT 7 Jan 2025 Nasdaq Composite leads with 1.2% gain, marking best start to year since 2009.";
    const CScript genesisOutputScript = CScript() 
        << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") 
        << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

// GetHashOfScashXGenesisBlock: computes the double-SHA256 hash of the block header (including RandomX fields).
static uint256 GetHashOfScashXGenesisBlock(const CBlock& genesis)
{
    CBlockHeader rx_blockHeader(genesis);
    unsigned char hash[32];
    // Pass a Span constructed from rx_blockHeader's bytes.
    CHash256().Write(Span<const unsigned char>{(unsigned char*)&rx_blockHeader, sizeof(rx_blockHeader)})
               .Finalize(hash);
    return uint256(hash);
}

// GetRandomXCommitment_: computes the RandomX commitment by temporarily clearing the hashRandomX field.
static uint256 GetRandomXCommitment_(const CBlockHeader& block)
{
    uint256 rx_hash = block.hashRandomX;
    CBlockHeader rx_blockHeader(block);
    rx_blockHeader.hashRandomX.SetNull();
    char rx_cm[RANDOMX_HASH_SIZE];
    randomx_calculate_commitment(&rx_blockHeader, sizeof(rx_blockHeader), rx_hash.data(), rx_cm);
    return uint256(std::vector<unsigned char>(rx_cm, rx_cm + sizeof(rx_cm)));
}

int main()
{
    // Genesis block parameters (example values)
    uint32_t nTime = 1708650456;
    //uint32_t nBits = 0x1e0fffff;     // ScashX mainnet (for genesis block)  First nonce: 395926
    //uint32_t nBits = 0x1d00ffff;     // Bitcoin testnet
    //uint32_t nBits = 0x1e7fffff;     // ScashX testnet
    // uint32_t nBits = 0x207fffff;    // ScashX regtest
    // uint32_t nBits = 0x1e0fffff; //0x1d00ffff; //0x20dfffff;   // options in the original scash code
    uint32_t nBits = 0x1e0fffff; 
    
    uint32_t nNonce = 20000000 ; // Start from 0  // ScashX genesis 20076863   // test start 390000 or 20000000
    int32_t nVersion = 1;
    CAmount genesisReward = 50 * COIN;

    // Flags used by SetCompact; not used further in this simple example.
    bool fNegative = false;
    bool fOverflow = false;

    // Create the genesis block.
    CBlock genesis = CreateScashXGenesisBlock(nTime, nNonce, nBits, nVersion, genesisReward);
    // Set an initial RandomX hash (this value might be adjusted or computed differently in a real scenario)
    genesis.hashRandomX = uint256S("33c450e0152826e3a8948b01464cf9182344a1544b3ddcf6153dd04b62938d01");

    // Compute the target from nBits.
    arith_uint256 bnTarget = arith_uint256().SetCompact(nBits, &fNegative, &fOverflow);

    std::cout << "Starting genesis block mining (RandomX POW)..." << std::endl;

    // Mining loop: iterate until the RandomX commitment meets the target.
    while (true)
    {
        genesis.nNonce = nNonce;
        uint256 hash = GetHashOfScashXGenesisBlock(genesis);
        uint256 rxCommitment = GetRandomXCommitment_(genesis);
/*
        std::cout << "Nonce: " << nNonce
                  << " | Genesis Hash: " << hash.ToString()
                  << " | RX Commitment: " << rxCommitment.ToString() << std::endl;
*/
        if (UintToArith256(rxCommitment) < bnTarget)
        {
            std::cout << "\nGenesis block found!" << std::endl;
            std::cout << "Nonce: " << nNonce << std::endl;
            std::cout << "Target: " << bnTarget.ToString() << std::endl;
            std::cout << "Genesis Hash: " << hash.ToString() << std::endl;
            std::cout << "Genesis RandomX Commitment: " << rxCommitment.ToString() << std::endl;
            std::cout << "Merkle Root: " << genesis.hashMerkleRoot.ToString() << std::endl;
            break;
        }
/* modify????
        else
        {
            if (nNonce % 1000000 == 0)
            {
                std::cout << "hash " << hash.ToString() 
                          << " bnTarget " << bnTarget.ToString() << std::endl;
            }
        }
*/
        nNonce++;
        if (nNonce % 1000000 == 0)
        {
            std::cout << "Tried " << nNonce << " nonces..." << std::endl;
        }
    }

    return 0;
}
