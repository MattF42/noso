// Copyright (c) 2014-2023 The Dash Core developers
// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/subsidy.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(subsidy_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();

    // Test initial subsidy (12 NOSOR)
    CAmount nSubsidy = GetTotalBlockSubsidy(0, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 12 * COIN);

    nSubsidy = GetTotalBlockSubsidy(1, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 12 * COIN);

    // Test before first halving
    nSubsidy = GetTotalBlockSubsidy(630719, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 12 * COIN);

    // Test at first halving (should be 6 NOSOR = 12 / 2)
    nSubsidy = GetTotalBlockSubsidy(630720, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 6 * COIN);

    nSubsidy = GetTotalBlockSubsidy(630721, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 6 * COIN);

    // Test before second halving
    nSubsidy = GetTotalBlockSubsidy(1261439, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 6 * COIN);

    // Test at second halving (should be 3 NOSOR = 6 / 2)
    nSubsidy = GetTotalBlockSubsidy(1261440, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 3 * COIN);

    // Test at third halving (should be 1.5 NOSOR = 3 / 2)
    nSubsidy = GetTotalBlockSubsidy(1892160, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 150000000); // 1.5 * COIN

    // Test at fourth halving (should be 0.75 NOSOR)
    nSubsidy = GetTotalBlockSubsidy(2522880, consensus);
    BOOST_CHECK_EQUAL(nSubsidy, 75000000); // 0.75 * COIN
}

BOOST_AUTO_TEST_CASE(subsidy_split_test)
{
    // Test split with 12 NOSOR initial subsidy
    CAmount nTotal = 12 * COIN;
    CAmount nMN, nMiner, nDev, nCommunity;
    
    GetSubsidySplit(nTotal, nMN, nMiner, nDev, nCommunity);
    
    // Check percentages
    BOOST_CHECK_EQUAL(nMN, 6 * COIN);        // 50%
    BOOST_CHECK_EQUAL(nMiner, (CAmount)(4.8 * COIN)); // 40%
    BOOST_CHECK_EQUAL(nDev, (CAmount)(0.12 * COIN));  // 1%
    
    // Community should get residual
    BOOST_CHECK_EQUAL(nCommunity, nTotal - (nMN + nMiner + nDev));
    
    // Total should match
    BOOST_CHECK_EQUAL(nMN + nMiner + nDev + nCommunity, nTotal);

    // Test split with 6 NOSOR (after first halving)
    nTotal = 6 * COIN;
    GetSubsidySplit(nTotal, nMN, nMiner, nDev, nCommunity);
    
    BOOST_CHECK_EQUAL(nMN, 3 * COIN);        // 50%
    BOOST_CHECK_EQUAL(nMiner, (CAmount)(2.4 * COIN)); // 40%
    BOOST_CHECK_EQUAL(nDev, (CAmount)(0.06 * COIN));  // 1%
    BOOST_CHECK_EQUAL(nMN + nMiner + nDev + nCommunity, nTotal);

    // Test split with odd number to verify rounding
    nTotal = 1000001; // 0.00001001 NOSOR
    GetSubsidySplit(nTotal, nMN, nMiner, nDev, nCommunity);
    
    BOOST_CHECK_EQUAL(nMN, 500000);      // floor(1000001 * 50 / 100)
    BOOST_CHECK_EQUAL(nMiner, 400000);   // floor(1000001 * 40 / 100)
    BOOST_CHECK_EQUAL(nDev, 10000);      // floor(1000001 * 1 / 100)
    BOOST_CHECK_EQUAL(nCommunity, 90001); // residual
    BOOST_CHECK_EQUAL(nMN + nMiner + nDev + nCommunity, nTotal);
}

BOOST_AUTO_TEST_CASE(total_supply_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    
    // Calculate total mined supply over many halvings
    CAmount totalMined = 0;
    int halvingInterval = consensus.nSubsidyHalvingInterval;
    
    // Sum up subsidies for first 10 halving periods
    for (int period = 0; period < 10; period++) {
        int startHeight = period * halvingInterval;
        CAmount subsidy = GetTotalBlockSubsidy(startHeight, consensus);
        totalMined += subsidy * halvingInterval;
    }
    
    // Add genesis premine
    totalMined += 9000000 * COIN;
    
    // With 12 NOSOR initial, halving every 630720 blocks:
    // Period 0: 12 * 630720 = 7,568,640 NOSOR
    // Period 1: 6 * 630720 = 3,784,320 NOSOR
    // Period 2: 3 * 630720 = 1,892,160 NOSOR
    // Period 3: 1.5 * 630720 = 946,080 NOSOR
    // ...continuing halving
    // Total mined (excluding premine) approaches 24,000,000 NOSOR
    // With 9M premine, max supply approaches 21M (due to rounding in halvings)
    
    // Verify total is reasonable (should be less than MAX_MONEY)
    BOOST_CHECK(totalMined <= MAX_MONEY);
}

BOOST_AUTO_TEST_SUITE_END()
