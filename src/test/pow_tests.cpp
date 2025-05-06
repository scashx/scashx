// Copyright (c) 2015-2022 The Bitcoin Core developers
// Copyright (c) 2024 The Scash developers
// Copyright (c) 2025 The Satoshi Cash-X developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

#include <cmath>


BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;

    // Here (and below): expected_nbits is calculated in
    // CalculateNextWorkRequired(); redoing the calculation here would be just
    // reimplementing the same code that is written in pow.cpp. Rather than
    // copy that code, we just hardcode the expected result.
    unsigned int expected_nbits = 0x1d00d86aU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    unsigned int expected_nbits = 0x1d00ffffU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    unsigned int expected_nbits = 0x1c0168fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that reducing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits-1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    unsigned int expected_nbits = 0x1d00e1fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that increasing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits+1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits{~0x00800000U};
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, ChainType chain_type)
{
    const auto chainParams = CreateChainParams(args, chain_type);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 4*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()

    if (g_isRandomX && !consensus.fPowNoRetargeting) {
        arith_uint512 targ_max_512("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
        targ_max_512 /= consensus.nPowTargetTimespan*4;
        arith_uint512 powLimit_512 = arith_uint512::from(UintToArith256(consensus.powLimit));
        BOOST_CHECK(powLimit_512 < targ_max_512);
    } else
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
        targ_max /= consensus.nPowTargetTimespan*4;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::SIGNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_SCASHXREGTEST_sanity)
{
    g_isRandomX = true;
    sanity_check_chainparams(*m_node.args, ChainType::SCASHXREGTEST);
    g_isRandomX = false;
}

BOOST_AUTO_TEST_CASE(ChainParams_SCASHXTESTNET_sanity)
{
    g_isRandomX = true;
    sanity_check_chainparams(*m_node.args, ChainType::SCASHXTESTNET);
    g_isRandomX = false;
}

BOOST_AUTO_TEST_CASE(ChainParams_SCASHXMAIN_sanity)
{
    g_isRandomX = true;
    sanity_check_chainparams(*m_node.args, ChainType::SCASHXMAIN);
    g_isRandomX = false;
}

BOOST_AUTO_TEST_CASE(Check_Epoch_Calculation)
{
    // Epoch is unix timestamp in seconds since 1970 divided by epoch duration in seconds
    BOOST_CHECK(0 / 10000 == GetEpoch(0, 10000));
    BOOST_CHECK(1600000000 / 1 == GetEpoch(1600000000, 1));
    BOOST_CHECK(1707328799 / 3600 == GetEpoch(1707328799, 3600));
    BOOST_CHECK_EQUAL(474257, GetEpoch(1707328799, 3600));
    BOOST_CHECK_EQUAL(474258, GetEpoch(1707328800, 3600));
    BOOST_CHECK_EQUAL(474258, GetEpoch(1707330114, 3600));
    BOOST_CHECK_EQUAL(474258, GetEpoch(1707332399, 3600));
    BOOST_CHECK_EQUAL(474259, GetEpoch(1707332400, 3600));
}

BOOST_AUTO_TEST_CASE(Check_RandomX_Key_Generation)
{
    // RandomX key is sha256d of seed string where the epoch number changes
    // "ScashX/RandomX/Epoch/1"
    uint256 hash = GetSeedHash(1);
    BOOST_CHECK_EQUAL(hash, uint256S("00dbf089477a1cd4ac7d64a81595ab22fe1e0e045954d0635f4b954bc3b3df00"));
    // "ScashX/RandomX/Epoch/999"
    hash = GetSeedHash(999);
    BOOST_CHECK_EQUAL(hash, uint256S("82107e0e65b970e0287a89f1afa78cc95a78bd755813ee481214152e295d634c"));
}

BOOST_AUTO_TEST_CASE(Check_RandomX_BlockHeader)
{
    m_node.args->ForceSetArg("-randomxfastmode", "0"); // disable fast mode which requires at least 2GB of memory
    
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::SCASHXTESTNET);
    const auto consensus = chainParams->GetConsensus();

    // Sanity check: block header GetHash() function includes RandomX field when running as ScashX
    assert(!g_isRandomX);
    BOOST_CHECK_NE(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());
    g_isRandomX = true;
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());
    g_isRandomX = false;

    // CheckProofOfWorkRandomX() checks if commitment (computed from block header) meets targett
    CBlockHeader block = chainParams->GenesisBlock().GetBlockHeader();
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_COMMITMENT_ONLY)); 
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_FULL));

    // Invalid if randomx hash is null, unless in mining mode
    block.hashRandomX.SetNull();
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_COMMITMENT_ONLY));
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_FULL));

    uint256 rx_hash;

    // If block header is valid, the optional outHash is set with the randomx hash
    block = chainParams->GenesisBlock().GetBlockHeader();
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_COMMITMENT_ONLY, &rx_hash));
    BOOST_CHECK_EQUAL(rx_hash, chainParams->GenesisBlock().hashRandomX);
    rx_hash.SetNull();
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_FULL, &rx_hash));
    BOOST_CHECK_EQUAL(rx_hash, chainParams->GenesisBlock().hashRandomX);

     // If block header is invalid, the optional outHash is not set with the randomx hash
    block.hashRandomX = uint256::ONE;
    rx_hash.SetNull();
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_COMMITMENT_ONLY, &rx_hash));
    BOOST_CHECK_NE(rx_hash, uint256::ONE);
    rx_hash.SetNull();
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_FULL, &rx_hash));
    BOOST_CHECK_NE(rx_hash, uint256::ONE);   

    // Mining requires the outHash parameter
    BOOST_CHECK_THROW(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING), std::runtime_error);
    BOOST_CHECK_THROW(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING, NULL), std::runtime_error);

    // Mining success: outHash is set, so miner can add to block header
    block = chainParams->GenesisBlock().GetBlockHeader();
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING, &rx_hash));
    BOOST_CHECK_EQUAL(rx_hash, chainParams->GenesisBlock().hashRandomX);
    block.hashRandomX.SetNull();    
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING, &rx_hash));
    BOOST_CHECK_EQUAL(rx_hash, chainParams->GenesisBlock().hashRandomX);

    // Mining fails: outHash parameter is not set
    block.nNonce = 123456;
    rx_hash.SetNull();
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING, &rx_hash));
    assert(rx_hash.IsNull());
    rx_hash = uint256(123);
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_MINING, &rx_hash));
    BOOST_CHECK_EQUAL(rx_hash, uint256(123));

    // Light verification can be useful when blocks are already known to be fully verified.
    // The trade-off is reduced security. For example, a RandomX hash value in the block header
    // can be chosen so that the commitment meets the target, even though the hash is invalid.
    // rx = 1cdd3b89dd7ff815d78f403c37d5d3ed853d6332beae928ed1c6526ccc969a05
    // cm = 00001b9288bf6532b764c93d689f86cc79b19458bb5ae27baa366bb3a792f5aa
    block = chainParams->GenesisBlock().GetBlockHeader();
    block.hashRandomX = uint256S("1cdd3b89dd7ff815d78f403c37d5d3ed853d6332beae928ed1c6526ccc969a05");
    assert(CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_COMMITMENT_ONLY));
    assert(!CheckProofOfWorkRandomX(block, consensus, POW_VERIFY_FULL));

    // Commitment calculation requires Randomx hash value in block header to be null
    block = chainParams->GenesisBlock().GetBlockHeader();
    rx_hash = block.hashRandomX;
	char rx_cm_bad[RANDOMX_HASH_SIZE];
    randomx_calculate_commitment(&block, sizeof(block), rx_hash.data(), rx_cm_bad);
    block.hashRandomX = uint256();
	char rx_cm[RANDOMX_HASH_SIZE];
    randomx_calculate_commitment(&block, sizeof(block), rx_hash.data(), rx_cm);
    assert(memcmp(rx_cm, rx_cm_bad, sizeof(rx_cm)) != 0);
    BOOST_CHECK_EQUAL(uint256(std::vector<unsigned char>(rx_cm, rx_cm + sizeof(rx_cm))), uint256S("00000922ba3e0d5f9aa758a22dd73165912c858f50673a34b07a3ccdbe6e8dcb"));

    // Basic tests of GetRandomXCommitment()
    block = chainParams->GenesisBlock();
    uint256 cm = GetRandomXCommitment(chainParams->GenesisBlock());
    BOOST_CHECK_EQUAL(cm, uint256S("00000922ba3e0d5f9aa758a22dd73165912c858f50673a34b07a3ccdbe6e8dcb"));
    cm = GetRandomXCommitment(chainParams->GenesisBlock(), NULL);
    BOOST_CHECK_EQUAL(cm, uint256S("00000922ba3e0d5f9aa758a22dd73165912c858f50673a34b07a3ccdbe6e8dcb"));
    // set inHash parameter
    cm = GetRandomXCommitment(chainParams->GenesisBlock(), &block.hashRandomX);
    BOOST_CHECK_EQUAL(cm, uint256S("00000922ba3e0d5f9aa758a22dd73165912c858f50673a34b07a3ccdbe6e8dcb"));
    rx_hash = uint256(123);
    cm = GetRandomXCommitment(chainParams->GenesisBlock(), &rx_hash);
    BOOST_CHECK_NE(cm, uint256S("00000922ba3e0d5f9aa758a22dd73165912c858f50673a34b07a3ccdbe6e8dcb"));
}


// !BITCOINCASH

/**
 * ASERT Difficulty Adjustment Algorithm (aserti3-2d)
 * https://reference.cash/protocol/forks/2020-11-15-asert
 * Source code:
 * https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/blob/0a5fa6246387c3a9498898ee5257ee6950c1b635/src/test/pow_tests.cpp
 *
 * Any changes to the Bitcoin Cash code because ScashX has a different powlimit are marked with ScashX guards.
 */

using CBlockIndexPtr = std::unique_ptr<CBlockIndex>;
const auto MkCBlockIndexPtr = &std::make_unique<CBlockIndex>;

static CBlockIndexPtr GetBlockIndex(CBlockIndex *pindexPrev, int64_t nTimeInterval,
                                    uint32_t nBits) {
    CBlockIndexPtr block = MkCBlockIndexPtr();
    block->pprev = pindexPrev;
    block->nHeight = pindexPrev->nHeight + 1;
    block->nTime = pindexPrev->nTime + nTimeInterval;
    block->nBits = nBits;

    block->BuildSkip();
    block->nChainWork = pindexPrev->nChainWork + GetBlockProof(*block);
    return block;
}

double TargetFromBits(const uint32_t nBits) {
    return (nBits & 0xff'ff'ff) * pow(256, (nBits >> 24)-3);
}

double GetASERTApproximationError(const CBlockIndex *pindexPrev,
                                  const uint32_t finalBits,
                                  const CBlockIndex *pindexAnchorBlock) {
    const int64_t nHeightDiff = pindexPrev->nHeight - pindexAnchorBlock->nHeight;
    const int64_t nTimeDiff   = pindexPrev->GetBlockTime()   - pindexAnchorBlock->pprev->GetBlockTime();
    const uint32_t initialBits = pindexAnchorBlock->nBits;

    BOOST_CHECK(nHeightDiff >= 0);
    double dInitialPow = TargetFromBits(initialBits);
    double dFinalPow   = TargetFromBits(finalBits);

    double dExponent = double(nTimeDiff - (nHeightDiff+1) * 600) / double(2*24*3600);
    double dTarget = dInitialPow * pow(2, dExponent);

    return (dFinalPow - dTarget) / dTarget;
}

BOOST_AUTO_TEST_CASE(asert_difficulty_test) {
    // Use BCH powLimit to replicate BCH tests
    Consensus::Params mutableParams = CreateChainParams(*m_node.args, ChainType::SCASHXMAIN)->GetConsensus();
    mutableParams.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    std::vector<CBlockIndexPtr> blocks(3000 + 2*24*3600);
    mutableParams.asertAnchorParams.reset();  // clear hard-coded anchor block so that we may perform these below tests
    const Consensus::Params &params = mutableParams; // take a const reference    
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    arith_uint256 currentPow = powLimit >> 3;
    uint32_t initialBits = currentPow.GetCompact();
    double dMaxErr = 0.0001166792656486;

    // Genesis block, and parent of ASERT anchor block in this test case.
    blocks[0] = MkCBlockIndexPtr();
    blocks[0]->nHeight = 0;
    blocks[0]->nTime = 1269211443;
    // The pre-anchor block's nBits should never be used, so we set it to a nonsense value in order to
    // trigger an error if it is ever accessed
    blocks[0]->nBits = 0x0dedbeef;

    blocks[0]->nChainWork = GetBlockProof(*blocks[0]);

    // Block counter.
    size_t i = 1;

    // ASERT anchor block. We give this one a solvetime of 150 seconds to ensure that
    // the solvetime between the pre-anchor and the anchor blocks is actually used.
    blocks[1] = GetBlockIndex(blocks[0].get(), 150, initialBits);
    // The nBits for the next block should not be equal to the anchor block's nBits
    CBlockHeader blkHeaderDummy;
    uint32_t nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[1].get())) < dMaxErr);
    BOOST_CHECK(nBits != initialBits);

    // If we add another block at 1050 seconds, we should return to the anchor block's nBits
    blocks[i] = GetBlockIndex(blocks[i-1].get(), 1050, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(nBits == initialBits);
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[1].get())) < dMaxErr);

    currentPow = arith_uint256().SetCompact(nBits);
    // Before we do anything else, check that timestamps *before* the anchor block work fine.
    // Jumping 2 days into the past will give a timestamp before the achnor, and should halve the target
    blocks[i] = GetBlockIndex(blocks[i-1].get(), 600-172800, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    currentPow = arith_uint256().SetCompact(nBits);
    // Because nBits truncates target, we don't end up with exactly 1/2 the target
    BOOST_CHECK(currentPow <= arith_uint256().SetCompact(initialBits  ) / 2);
    BOOST_CHECK(currentPow >= arith_uint256().SetCompact(initialBits-1) / 2);
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[1].get())) < dMaxErr);

    // Jumping forward 2 days should return the target to the initial value
    blocks[i] = GetBlockIndex(blocks[i-1].get(), 600+172800, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    currentPow = arith_uint256().SetCompact(nBits);
    BOOST_CHECK(nBits == initialBits);
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[1].get())) < dMaxErr);

    // Pile up some blocks every 10 mins to establish some history.
    for (; i < 150; i++) {
        blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600, nBits);
        BOOST_CHECK_EQUAL(blocks[i]->nBits, nBits);
    }

    nBits = GetNextASERTWorkRequired(blocks[i - 1].get(), &blkHeaderDummy, params, blocks[1].get());

    BOOST_CHECK_EQUAL(nBits, initialBits);

    // Difficulty stays the same as long as we produce a block every 10 mins.
    for (size_t j = 0; j < 10; i++, j++) {
        blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600, nBits);
        BOOST_CHECK_EQUAL(
            GetNextASERTWorkRequired(blocks[i].get(), &blkHeaderDummy, params, blocks[1].get()),
            nBits);
    }

    // If we add a two blocks whose solvetimes together add up to 1200s,
    // then the next block's target should be the same as the one before these blocks
    // (at this point, equal to initialBits).
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 300, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr);
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 900, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    BOOST_CHECK_EQUAL(nBits, initialBits);
    BOOST_CHECK(nBits != blocks[i-1]->nBits);

    // Same in reverse - this time slower block first, followed by faster block.
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 900, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 300, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    BOOST_CHECK_EQUAL(nBits, initialBits);
    BOOST_CHECK(nBits != blocks[i-1]->nBits);

    // Jumping forward 2 days should double the target (halve the difficulty)
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600 + 2*24*3600, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    currentPow = arith_uint256().SetCompact(nBits) / 2;
    BOOST_CHECK_EQUAL(currentPow.GetCompact(), initialBits);

    // Jumping backward 2 days should bring target back to where we started
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600 - 2*24*3600, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    BOOST_CHECK_EQUAL(nBits, initialBits);

    // Jumping backward 2 days should halve the target (double the difficulty)
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600 - 2*24*3600, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    currentPow = arith_uint256().SetCompact(nBits);
    // Because nBits truncates target, we don't end up with exactly 1/2 the target
    BOOST_CHECK(currentPow <= arith_uint256().SetCompact(initialBits  ) / 2);
    BOOST_CHECK(currentPow >= arith_uint256().SetCompact(initialBits-1) / 2);

    // And forward again
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600 + 2*24*3600, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    BOOST_CHECK_EQUAL(nBits, initialBits);
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), 600 + 2*24*3600, nBits);
    nBits = GetNextASERTWorkRequired(blocks[i++].get(), &blkHeaderDummy, params, blocks[1].get());
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[ 1 ].get())) < dMaxErr); // absolute
    BOOST_CHECK(fabs(GetASERTApproximationError(blocks[i-1].get(), nBits, blocks[i-2].get())) < dMaxErr); // relative
    currentPow = arith_uint256().SetCompact(nBits) / 2;
    BOOST_CHECK_EQUAL(currentPow.GetCompact(), initialBits);

    // Iterate over the entire -2*24*3600..+2*24*3600 range to check that our integer approximation:
    //   1. Should be monotonic
    //   2. Should change target at least once every 8 seconds (worst-case: 15-bit precision on nBits)
    //   3. Should never change target by more than XXXX per 1-second step
    //   4. Never exceeds dMaxError in absolute error vs a double float calculation
    //   5. Has almost exactly the dMax and dMin errors we expect for the formula
    double dMin = 0;
    double dMax = 0;
    double dErr;
    double dRelMin = 0;
    double dRelMax = 0;
    double dRelErr;
    double dMaxStep = 0;
    uint32_t nBitsRingBuffer[8];
    double dStep = 0;
    blocks[i] = GetBlockIndex(blocks[i - 1].get(), -2*24*3600 - 30, nBits);
    for (size_t j = 0; j < 4*24*3600 + 660; j++) {
        blocks[i]->nTime++;
        nBits = GetNextASERTWorkRequired(blocks[i].get(), &blkHeaderDummy, params, blocks[1].get());

        if (j > 8) {
            // 1: Monotonic
            BOOST_CHECK(arith_uint256().SetCompact(nBits) >= arith_uint256().SetCompact(nBitsRingBuffer[(j-1)%8]));
            // 2: Changes at least once every 8 seconds (worst case: nBits = 1d008000 to 1d008001)
            BOOST_CHECK(arith_uint256().SetCompact(nBits) > arith_uint256().SetCompact(nBitsRingBuffer[j%8]));
            // 3: Check 1-sec step size
            dStep = (TargetFromBits(nBits) - TargetFromBits(nBitsRingBuffer[(j-1)%8])) / TargetFromBits(nBits);
            if (dStep > dMaxStep) dMaxStep = dStep;
            BOOST_CHECK(dStep < 0.0000314812106363); // from nBits = 1d008000 to 1d008001
        }
        nBitsRingBuffer[j%8] = nBits;

        // 4 and 5: check error vs double precision float calculation
        dErr    = GetASERTApproximationError(blocks[i].get(), nBits, blocks[1].get());
        dRelErr = GetASERTApproximationError(blocks[i].get(), nBits, blocks[i-1].get());
        if (dErr    < dMin)    dMin    = dErr;
        if (dErr    > dMax)    dMax    = dErr;
        if (dRelErr < dRelMin) dRelMin = dRelErr;
        if (dRelErr > dRelMax) dRelMax = dRelErr;
        BOOST_CHECK_MESSAGE(fabs(dErr) < dMaxErr,
                            strprintf("solveTime: %d\tStep size: %.8f%%\tdErr: %.8f%%\tnBits: %0x\n",
                                      int64_t(blocks[i]->nTime) - blocks[i-1]->nTime, dStep*100, dErr*100, nBits));
        BOOST_CHECK_MESSAGE(fabs(dRelErr) < dMaxErr,
                            strprintf("solveTime: %d\tStep size: %.8f%%\tdRelErr: %.8f%%\tnBits: %0x\n",
                                      int64_t(blocks[i]->nTime) - blocks[i-1]->nTime, dStep*100, dRelErr*100, nBits));
    }
    auto failMsg = strprintf("Min error: %16.14f%%\tMax error: %16.14f%%\tMax step: %16.14f%%\n", dMin*100, dMax*100, dMaxStep*100);
    BOOST_CHECK_MESSAGE(   dMin < -0.0001013168981059
                        && dMin > -0.0001013168981060
                        && dMax >  0.0001166792656485
                        && dMax <  0.0001166792656486,
                        failMsg);
    failMsg = strprintf("Min relError: %16.14f%%\tMax relError: %16.14f%%\n", dRelMin*100, dRelMax*100);
    BOOST_CHECK_MESSAGE(   dRelMin < -0.0001013168981059
                        && dRelMin > -0.0001013168981060
                        && dRelMax >  0.0001166792656485
                        && dRelMax <  0.0001166792656486,
                        failMsg);

    // Difficulty increases as long as we produce fast blocks
    for (size_t j = 0; j < 100; i++, j++) {
        uint32_t nextBits;
        arith_uint256 currentTarget;
        currentTarget.SetCompact(nBits);

        blocks[i] = GetBlockIndex(blocks[i - 1].get(), 500, nBits);
        nextBits = GetNextASERTWorkRequired(blocks[i].get(), &blkHeaderDummy, params, blocks[1].get());
        arith_uint256 nextTarget;
        nextTarget.SetCompact(nextBits);

        // Make sure that target is decreased
        BOOST_CHECK(nextTarget <= currentTarget);

        nBits = nextBits;
    }

}

std::string StrPrintCalcArgs(const arith_uint256 refTarget,
                             const int64_t targetSpacing,
                             const int64_t timeDiff,
                             const int64_t heightDiff,
                             const arith_uint256 expectedTarget,
                             const uint32_t expectednBits) {
    return strprintf("\n"
                     "ref=         %s\n"
                     "spacing=     %d\n"
                     "timeDiff=    %d\n"
                     "heightDiff=  %d\n"
                     "expTarget=   %s\n"
                     "exp nBits=   0x%08x\n",
                     refTarget.ToString(),
                     targetSpacing,
                     timeDiff,
                     heightDiff,
                     expectedTarget.ToString(),
                     expectednBits);
}


// Tests of the CalculateASERT function.
BOOST_AUTO_TEST_CASE(calculate_asert_test) {
    // Use BCH powLimit to replicate BCH tests
    Consensus::Params params = CreateChainParams(*m_node.args, ChainType::SCASHXMAIN)->GetConsensus();
    params.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    const int64_t nHalfLife = params.nASERTHalfLife;

    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    arith_uint256 initialTarget = powLimit >> 4;
    int64_t height = 0;

    // The CalculateASERT function uses the absolute ASERT formulation
    // and adds +1 to the height difference that it receives.
    // The time difference passed to it must factor in the difference
    // to the *parent* of the reference block.
    // We assume the parent is ideally spaced in time before the reference block.
    static const int64_t parent_time_diff = 600;

    // Steady
    arith_uint256 nextTarget = CalculateASERT(initialTarget, params.nPowTargetSpacing, parent_time_diff + 600 /* nTimeDiff */, ++height, powLimit, nHalfLife);
    BOOST_CHECK(nextTarget == initialTarget);

    // A block that arrives in half the expected time
    nextTarget = CalculateASERT(initialTarget, params.nPowTargetSpacing, parent_time_diff + 600 + 300, ++height, powLimit, nHalfLife);
    BOOST_CHECK(nextTarget < initialTarget);

    // A block that makes up for the shortfall of the previous one, restores the target to initial
    arith_uint256 prevTarget = nextTarget;
    nextTarget = CalculateASERT(initialTarget, params.nPowTargetSpacing, parent_time_diff + 600 + 300 + 900, ++height, powLimit, nHalfLife);
    BOOST_CHECK(nextTarget > prevTarget);
    BOOST_CHECK(nextTarget == initialTarget);

    // Two days ahead of schedule should double the target (halve the difficulty)
    prevTarget = nextTarget;
    nextTarget = CalculateASERT(prevTarget, params.nPowTargetSpacing, parent_time_diff + 288*1200, 288, powLimit, nHalfLife);
    BOOST_CHECK(nextTarget == prevTarget * 2);

    // Two days behind schedule should halve the target (double the difficulty)
    prevTarget = nextTarget;
    nextTarget = CalculateASERT(prevTarget, params.nPowTargetSpacing, parent_time_diff + 288*0, 288, powLimit, nHalfLife);
    BOOST_CHECK(nextTarget == prevTarget / 2);
    BOOST_CHECK(nextTarget == initialTarget);

    // Ramp up from initialTarget to PowLimit - should only take 4 doublings...
    uint32_t powLimit_nBits = powLimit.GetCompact();
    uint32_t next_nBits;
    for (size_t k = 0; k < 3; k++) {
        prevTarget = nextTarget;
        nextTarget = CalculateASERT(prevTarget, params.nPowTargetSpacing, parent_time_diff + 288*1200, 288, powLimit, nHalfLife);
        BOOST_CHECK(nextTarget == prevTarget * 2);
        BOOST_CHECK(nextTarget < powLimit);
        next_nBits = nextTarget.GetCompact();
        BOOST_CHECK(next_nBits != powLimit_nBits);
    }

    prevTarget = nextTarget;
    nextTarget = CalculateASERT(prevTarget, params.nPowTargetSpacing, parent_time_diff + 288*1200, 288, powLimit, nHalfLife);
    next_nBits = nextTarget.GetCompact();
    BOOST_CHECK(nextTarget == prevTarget * 2);
    BOOST_CHECK(next_nBits == powLimit_nBits);

    // Fast periods now cannot increase target beyond POW limit, even if we try to overflow nextTarget.
    // prevTarget is a uint256, so 256*2 = 512 days would overflow nextTarget unless CalculateASERT
    // correctly detects this error
    nextTarget = CalculateASERT(prevTarget, params.nPowTargetSpacing, parent_time_diff + 512*144*600, 0, powLimit, nHalfLife);
    next_nBits = nextTarget.GetCompact();
    BOOST_CHECK(next_nBits == powLimit_nBits);

    // We also need to watch for underflows on nextTarget. We need to withstand an extra ~446 days worth of blocks.
    // This should bring down a powLimit target to the a minimum target of 1.
    nextTarget = CalculateASERT(powLimit, params.nPowTargetSpacing, 0, 2*(256-33)*144, powLimit, nHalfLife);
    next_nBits = nextTarget.GetCompact();
    BOOST_CHECK_EQUAL(next_nBits, arith_uint256(1).GetCompact());

    // Define a structure holding parameters to pass to CalculateASERT.
    // We are going to check some expected results  against a vector of
    // possible arguments.
    struct calc_params {
        arith_uint256 refTarget;
        int64_t targetSpacing;
        int64_t timeDiff;
        int64_t heightDiff;
        arith_uint256 expectedTarget;
        uint32_t expectednBits;
    };

    // Define some named input argument values
    const arith_uint256 SINGLE_300_TARGET { "00000000ffb1ffffffffffffffffffffffffffffffffffffffffffffffffffff" };
    const arith_uint256 FUNNY_REF_TARGET { "000000008000000000000000000fffffffffffffffffffffffffffffffffffff" };

    // Define our expected input and output values.
    // The timeDiff entries exclude the `parent_time_diff` - this is
    // added in the call to CalculateASERT in the test loop.
    const std::vector<calc_params> calculate_args = {

        /* refTarget, targetSpacing, timeDiff, heightDiff, expectedTarget, expectednBits */

        { powLimit, 600, 0, 2*144, powLimit >> 1, 0x1c7fffff },
        { powLimit, 600, 0, 4*144, powLimit >> 2, 0x1c3fffff },
        { powLimit >> 1, 600, 0, 2*144, powLimit >> 2, 0x1c3fffff },
        { powLimit >> 2, 600, 0, 2*144, powLimit >> 3, 0x1c1fffff },
        { powLimit >> 3, 600, 0, 2*144, powLimit >> 4, 0x1c0fffff },
        { powLimit, 600, 0, 2*(256-34)*144, 3, 0x01030000 },
        { powLimit, 600, 0, 2*(256-34)*144 + 119, 3, 0x01030000 },
        { powLimit, 600, 0, 2*(256-34)*144 + 120, 2, 0x01020000 },
        { powLimit, 600, 0, 2*(256-33)*144-1, 2, 0x01020000 },
        { powLimit, 600, 0, 2*(256-33)*144, 1, 0x01010000 },  // 1 bit less since we do not need to shift to 0
        { powLimit, 600, 0, 2*(256-32)*144, 1, 0x01010000 },  // more will not decrease below 1
        { 1, 600, 0, 2*(256-32)*144, 1, 0x01010000 },
        { powLimit, 600, 2*(512-32)*144, 0, powLimit, powLimit_nBits },
        { 1, 600, (512-64)*144*600, 0, powLimit, powLimit_nBits },
        { powLimit, 600, 300, 1, SINGLE_300_TARGET, 0x1d00ffb1 },  // clamps to powLimit
        { FUNNY_REF_TARGET, 600, 600*2*33*144, 0, powLimit, powLimit_nBits }, // confuses any attempt to detect overflow by inspecting result
        { 1, 600, 600*2*256*144, 0, powLimit, powLimit_nBits }, // overflow to exactly 2^256
        { 1, 600, 600*2*224*144 - 1, 0, arith_uint256(0xffff8) << 204, powLimit_nBits }, // just under powlimit (not clamped) yet over powlimit_nbits
    };

    for (auto& v : calculate_args) {
        nextTarget = CalculateASERT(v.refTarget, v.targetSpacing, parent_time_diff + v.timeDiff, v.heightDiff, powLimit, nHalfLife);
        next_nBits = nextTarget.GetCompact();
        const auto failMsg =
            StrPrintCalcArgs(v.refTarget, v.targetSpacing, parent_time_diff + v.timeDiff, v.heightDiff, v.expectedTarget, v.expectednBits)
            + strprintf("nextTarget=  %s\nnext nBits=  0x%08x\n", nextTarget.ToString(), next_nBits);
        BOOST_CHECK_MESSAGE(nextTarget == v.expectedTarget && next_nBits == v.expectednBits, failMsg);
    }
}

/**
 * Test transition of legacy Bitcoin DAA to ASERT algorithm with anchor block.
 */
BOOST_AUTO_TEST_CASE(asert_activation_anchor_scashx_test) {
    Consensus::Params params = CreateChainParams(*m_node.args, ChainType::SCASHXMAIN)->GetConsensus();
    params.asertAnchorParams.reset(); // clear hard-coded anchor block so that we may test the activation below
    CBlockHeader blkHeaderDummy;

    // an arbitrary compact target for our chain (based on ScashX chain ~ Apr 26 2024).
    uint32_t initialBits = 0x1c7b9d90;

    // Block store for anonymous blocks; needs to be big enough to fit all generated blocks in this test.
    std::vector<CBlockIndexPtr> blocks(10000);
    int bidx = 1;

    // Genesis block.
    blocks[0] = MkCBlockIndexPtr();
    blocks[0]->nHeight = 0;
    blocks[0]->nTime = 0;
    blocks[0]->nBits = initialBits;
    blocks[0]->nChainWork = GetBlockProof(*blocks[0]);

    // We want to create 2 Legacy DAA periods worth of blocks on schedule
    for (int i = 1; i < 4032; i++) {
        blocks[bidx] = GetBlockIndex(blocks[bidx-1].get(), 600, initialBits);
        bidx++;
        BOOST_REQUIRE(bidx < int(blocks.size()));
    }

    // Verify that under legacy DAA, block 4032 wuold not change difficulty, since blocks mined on schedule.
    BOOST_CHECK(bidx == 4032);
    CBlockIndex *pindexPreActivation = blocks[bidx-1].get();
    g_isRandomX = 1; // fixes off by 1
    BOOST_CHECK_EQUAL(GetNextWorkRequired(pindexPreActivation, &blkHeaderDummy, params), initialBits);
    g_isRandomX = 0;

    // Activate Asert DAA at block 4032, using block 1 as anchor and genesis block timestamp
    params.nASERTActivationHeight = 4032;
    params.asertAnchorParams = Consensus::Params::ASERTAnchor{
        1,                               // anchor block height
        initialBits,                     // anchor block nBits
        blocks[0].get()->GetBlockTime(), // anchor block previous block timestamp
    };

    // Verify that under ASERT DAA, diffculty remains the same. since blocks mined on schedule
    BOOST_CHECK_EQUAL(GetNextWorkRequired(pindexPreActivation, &blkHeaderDummy, params), initialBits);

    // Change schedule so block 4031 was mined at same time as block 4030, so blocks came in faster than expected
    blocks[4031].get()->nTime = blocks[4030].get()->GetBlockTime();

    // Verify that under ASERT DAA, difficulty increases for block 4032 when activating
    uint32_t nextBits = GetNextWorkRequired(pindexPreActivation, &blkHeaderDummy, params);
    BOOST_CHECK(nextBits < initialBits);
    BOOST_CHECK_EQUAL(nextBits, 0x1C7B51FE);

    // Mine block 4032
    blocks[4032] = GetBlockIndex(blocks[4031].get(), 1, nextBits);

    // Mine more blocks at 1 second interval, so that blocks are arriving too quickly.
    // Verify ASERT DAA is increasing the difficulty for every new block.
    for (bidx = 4033; bidx < 4100; bidx++) {
        auto pindexPrev = blocks[bidx-1].get();
        uint32_t prevBits = pindexPrev->nBits;
        nextBits = GetNextWorkRequired(pindexPrev, &blkHeaderDummy, params);
        BOOST_CHECK_NE(prevBits, nextBits);
        BOOST_CHECK(nextBits < prevBits);
        blocks[bidx] = GetBlockIndex(pindexPrev, 1, nextBits);
        BOOST_REQUIRE(bidx < int(blocks.size()));
    }

    // Mine next block, making it very slow e.g. 100 days to mine
    blocks[bidx] = GetBlockIndex(blocks[bidx-1].get(), 100 * 24 * 60 * 60, blocks[bidx-1].get()->nBits);

    // Verify ASERT reduces the following block has difficulty reduced to powlimit
    nextBits = GetNextWorkRequired(blocks[bidx].get(), &blkHeaderDummy, params);
    const uint32_t powLimit_nBits = UintToArith256(params.powLimit).GetCompact();
    BOOST_CHECK(powLimit_nBits == nextBits);
}

// !BITCOINCASH END

BOOST_AUTO_TEST_SUITE_END()
