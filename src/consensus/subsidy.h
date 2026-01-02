// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_SUBSIDY_H
#define BITCOIN_CONSENSUS_SUBSIDY_H

#include <consensus/amount.h>
#include <consensus/params.h>

struct SubsidySplit {
    CAmount masternode;  // 50%
    CAmount miner;       // 40%
    CAmount devfee;      // 1%
    CAmount community;   // 9%
};

/**
 * Calculate block subsidy for NOSOR emission schedule.
 * - Initial subsidy: 12 COIN
 * - Halving every 630,720 blocks
 * - Genesis block has premine of 9,000,000 COIN
 * 
 * @param nHeight Block height
 * @param consensusParams Consensus parameters
 * @return Total block subsidy (before split)
 */
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

/**
 * Split block subsidy according to NOSOR distribution:
 * - 50% to masternode
 * - 40% to miner
 * - 1% to dev fund
 * - 9% to community fund
 * 
 * Uses deterministic floor rounding: masternode, miner, and dev are floor(total*percentage/100),
 * community gets the remainder to ensure exact total.
 * 
 * @param nTotalSubsidy Total block subsidy
 * @return SubsidySplit structure with amounts for each recipient
 */
SubsidySplit GetSubsidySplit(CAmount nTotalSubsidy);

#endif // BITCOIN_CONSENSUS_SUBSIDY_H
