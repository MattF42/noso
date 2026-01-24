// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_COINBASE_CHECKS_H
#define BITCOIN_CONSENSUS_COINBASE_CHECKS_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/script.h>

#include <string>

class CBlockIndex;
namespace Consensus {
    struct Params;
}

/**
 * Check that coinbase outputs match expected subsidy split and payees.
 * 
 * Validates:
 * - Total coinbase value doesn't exceed block subsidy
 * - Proper distribution to miner, masternode, dev fund, and community fund
 * - Correct scriptPubKeys for dev and community funds
 * 
 * @param tx Coinbase transaction to validate
 * @param pindex Block index for context
 * @param consensusParams Consensus parameters
 * @param[out] strError Error message if validation fails
 * @return true if coinbase is valid, false otherwise
 */
bool CheckCoinbaseSubsidyAndPayees(const CTransaction& tx, const CBlockIndex* pindex, 
                                   const Consensus::Params& consensusParams, std::string& strError);

/**
 * Get the DF P2WPKH scriptPubKey used for dev fee.
 * Placeholder: 0014697cd07c801b8bba094f759de4fe742a6bae0470
 * 
 * @return CScript containing the P2WPKH scriptPubKey
 */
CScript GetDFScriptPubKey();

/**
 * Get the community fund scriptPubKey from consensus parameters.
 * On testnet/regtest/devnet: P2WSH 2-of-3 multisig
 * On mainnet: Same as devfee placeholder
 * 
 * @param consensusParams Consensus parameters
 * @return CScript containing the community fund scriptPubKey
 */
CScript GetCommunityFundScriptPubKey(const Consensus::Params& consensusParams);

#endif // BITCOIN_CONSENSUS_COINBASE_CHECKS_H
