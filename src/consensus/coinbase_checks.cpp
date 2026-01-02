// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/coinbase_checks.h>

#include <chain.h>
#include <consensus/params.h>
#include <consensus/subsidy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <vector>

// DF (Development Fund) temporary P2WPKH scriptPubKey
// Both DevFee (1%) and CommunityFee (9%) pay to this address until replaced by multisig
// P2WPKH scriptPubKey hex: 0014697cd07c801b8bba094f759de4fe742a6bae0470
static const std::vector<unsigned char> DF_SCRIPT_PUBKEY = ParseHex("0014697cd07c801b8bba094f759de4fe742a6bae0470");

bool CheckCoinbaseSubsidyAndPayees(const CTransaction& tx,
                                   const CBlockIndex* pindex,
                                   CAmount nTotalSubsidy,
                                   CAmount nFees,
                                   const Consensus::Params& params,
                                   std::string& strErrorRet)
{
    // Skip check for genesis block
    if (pindex == nullptr || pindex->nHeight == 0) {
        return true;
    }

    // Calculate the subsidy split
    CAmount nMasternodeSubsidy, nMinerSubsidy, nDevFee, nCommunityFee;
    GetSubsidySplit(nTotalSubsidy, nMasternodeSubsidy, nMinerSubsidy, nDevFee, nCommunityFee);

    // Find dev fee and community fee outputs in the coinbase
    bool foundDevFee = false;
    bool foundCommunityFee = false;
    CAmount totalPaidOut = 0;

    CScript dfScriptPubKey(DF_SCRIPT_PUBKEY.begin(), DF_SCRIPT_PUBKEY.end());

    for (const auto& output : tx.vout) {
        totalPaidOut += output.nValue;
        
        // Check if this output pays to the DF scriptPubKey
        if (output.scriptPubKey == dfScriptPubKey) {
            if (output.nValue == nDevFee) {
                foundDevFee = true;
            } else if (output.nValue == nCommunityFee) {
                foundCommunityFee = true;
            } else if (output.nValue == nDevFee + nCommunityFee) {
                // Allow combined payment for convenience
                foundDevFee = true;
                foundCommunityFee = true;
            }
        }
    }

    // Verify dev fee and community fee are present
    if (!foundDevFee) {
        strErrorRet = strprintf("Coinbase missing dev fee payment of %d satoshis to DF address", nDevFee);
        return false;
    }

    if (!foundCommunityFee) {
        strErrorRet = strprintf("Coinbase missing community fee payment of %d satoshis to DF address", nCommunityFee);
        return false;
    }

    // Total coinbase output cannot exceed total subsidy + fees
    CAmount nMaxAllowed = nTotalSubsidy + nFees;
    if (totalPaidOut > nMaxAllowed) {
        strErrorRet = strprintf("Coinbase pays too much (pays %d, max allowed %d)", totalPaidOut, nMaxAllowed);
        return false;
    }

    // Note: Masternode payment validation is handled by the existing masternode payment system
    // Note: Miner can claim their share (nMinerSubsidy) + fees, validated implicitly above

    return true;
}
