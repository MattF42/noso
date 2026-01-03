// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/subsidy.h>
#include <consensus/consensus.h>

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    // Genesis block has premine
    if (nHeight == 0) {
        return 9'000'000 * COIN;
    }

    // Initial subsidy: 12 COIN
    CAmount nSubsidy = 12 * COIN;
    
    // Halving every nSubsidyHalvingInterval blocks (630,720)
    int nHalvings = nHeight / consensusParams.nSubsidyHalvingInterval;
    
    // Subsidy is halved (divided by 2) for each halving interval
    // This effectively reduces emission by 50% per period
    nSubsidy >>= nHalvings;
    
    return nSubsidy;
}

SubsidySplit GetSubsidySplit(CAmount nTotalSubsidy)
{
    SubsidySplit split;
    
    // Use deterministic floor rounding
    // masternode = floor(total * 50 / 100)
    split.masternode = (nTotalSubsidy * 50) / 100;
    
    // miner = floor(total * 40 / 100)
    split.miner = (nTotalSubsidy * 40) / 100;
    
    // devfee = floor(total * 1 / 100)
    split.devfee = nTotalSubsidy / 100;
    
    // community = remainder to ensure exact total
    split.community = nTotalSubsidy - (split.masternode + split.miner + split.devfee);
    
    return split;
}
