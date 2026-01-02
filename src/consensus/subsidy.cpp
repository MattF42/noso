// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/subsidy.h>

#include <consensus/params.h>

CAmount GetTotalBlockSubsidy(int nHeight, const Consensus::Params& params)
{
    // Initial block subsidy is 12 NOSOR
    static const CAmount nInitialSubsidy = 12 * COIN;
    
    // Calculate number of halvings that have occurred
    int nHalvings = nHeight / params.nSubsidyHalvingInterval;
    
    // If too many halvings, subsidy is zero
    if (nHalvings >= 64) {
        return 0;
    }
    
    // Classic 50% halving: divide by 2 for each halving period
    CAmount nSubsidy = nInitialSubsidy;
    nSubsidy >>= nHalvings;  // Right shift is equivalent to dividing by 2^nHalvings
    
    return nSubsidy;
}

void GetSubsidySplit(CAmount nTotalSubsidy,
                     CAmount& nMasternodeSubsidy,
                     CAmount& nMinerSubsidy,
                     CAmount& nDevFee,
                     CAmount& nCommunityFee)
{
    // Calculate shares using integer math (floor division)
    // 50% to Masternodes
    nMasternodeSubsidy = (nTotalSubsidy * 50) / 100;
    
    // 40% to Miner
    nMinerSubsidy = (nTotalSubsidy * 40) / 100;
    
    // 1% to DevFee
    nDevFee = (nTotalSubsidy * 1) / 100;
    
    // Community gets the residual to ensure exact total
    // This accounts for any rounding from integer division
    nCommunityFee = nTotalSubsidy - (nMasternodeSubsidy + nMinerSubsidy + nDevFee);
}
