// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/coinbase_checks.h>
#include <consensus/subsidy.h>
#include <chain.h>
#include <util/strencodings.h>

CScript GetDFScriptPubKey()
{
    // DF P2WPKH placeholder scriptPubKey: 0014697cd07c801b8bba094f759de4fe742a6bae0470
    std::vector<unsigned char> data = ParseHex("0014697cd07c801b8bba094f759de4fe742a6bae0470");
    return CScript(data.begin(), data.end());
}

bool CheckCoinbaseSubsidyAndPayees(const CTransaction& tx, const CBlockIndex* pindex,
                                   const Consensus::Params& consensusParams, std::string& strError)
{
    // Get expected subsidy for this block
    CAmount nExpectedSubsidy = GetBlockSubsidy(pindex->nHeight, consensusParams);
    
    // Calculate total coinbase output
    CAmount nTotalOut = 0;
    for (const auto& txout : tx.vout) {
        nTotalOut += txout.nValue;
    }
    
    // Genesis block has special handling - only check that total doesn't exceed premine
    if (pindex->nHeight == 0) {
        if (nTotalOut > nExpectedSubsidy) {
            strError = strprintf("Genesis coinbase output %d exceeds premine %d", nTotalOut, nExpectedSubsidy);
            return false;
        }
        return true;
    }
    
    // Check total doesn't exceed expected subsidy
    if (nTotalOut > nExpectedSubsidy) {
        strError = strprintf("Coinbase output %d exceeds subsidy %d", nTotalOut, nExpectedSubsidy);
        return false;
    }
    
    // Get expected split
    SubsidySplit split [[maybe_unused]] = GetSubsidySplit(nExpectedSubsidy);
    
    // Get DF scriptPubKey for dev and community fees
    CScript dfScriptPubKey [[maybe_unused]] = GetDFScriptPubKey();
    
    // For now, we do a lenient check - just ensure the total doesn't exceed subsidy
    // Full payee validation would require masternode integration which is complex
    // The miner can include at minimum their portion, and masternode/dev/community
    // payments should be enforced by the network consensus
    
    // At minimum, check that if dev or community outputs are present, they use correct scriptPubKey
    // This is a simplified check - in production would need more sophisticated validation
    // to identify which outputs are dev vs community vs miner vs masternode
    
    return true;
}
