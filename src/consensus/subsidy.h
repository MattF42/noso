// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_SUBSIDY_H
#define BITCOIN_CONSENSUS_SUBSIDY_H

#include <consensus/amount.h>

namespace Consensus {
struct Params;
}

/**
 * Calculate the total block subsidy (before any splits) for a given block height.
 * 
 * Initial subsidy: 12 * COIN
 * Halving: Classic 50% halving every nSubsidyHalvingInterval blocks
 * 
 * @param nHeight Block height
 * @param params Consensus parameters containing nSubsidyHalvingInterval
 * @return Total block subsidy in satoshis
 */
CAmount GetTotalBlockSubsidy(int nHeight, const Consensus::Params& params);

/**
 * Split the total block subsidy according to the NOSOR emission schedule:
 * - 50% Masternodes
 * - 40% Miner (PoW)
 * - 1% DevFee
 * - 9% CommunityFee (receives residual to ensure exact total)
 * 
 * @param nTotalSubsidy Total block subsidy to split
 * @param nMasternodeSubsidy[out] Masternode portion (50%)
 * @param nMinerSubsidy[out] Miner portion (40%)
 * @param nDevFee[out] Dev fee portion (1%)
 * @param nCommunityFee[out] Community fee portion (9% + residual)
 */
void GetSubsidySplit(CAmount nTotalSubsidy,
                     CAmount& nMasternodeSubsidy,
                     CAmount& nMinerSubsidy,
                     CAmount& nDevFee,
                     CAmount& nCommunityFee);

#endif // BITCOIN_CONSENSUS_SUBSIDY_H
