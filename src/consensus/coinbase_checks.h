// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_COINBASE_CHECKS_H
#define BITCOIN_CONSENSUS_COINBASE_CHECKS_H

#include <consensus/amount.h>
#include <primitives/transaction.h>

#include <string>

class CBlockIndex;

namespace Consensus {
struct Params;
}

/**
 * Check that the coinbase transaction pays the correct subsidy amounts
 * to the correct recipients according to the NOSOR emission schedule.
 * 
 * This validates:
 * - Dev fee (1%) goes to the DF P2WPKH scriptPubKey
 * - Community fee (9%) goes to the DF P2WPKH scriptPubKey
 * - Masternode payment (50%) is validated separately by masternode payment logic
 * - Miner can claim up to their share (40%) plus transaction fees
 * 
 * @param tx The coinbase transaction to check
 * @param pindex Block index being validated
 * @param nTotalSubsidy Total block subsidy for this height
 * @param nFees Transaction fees in the block
 * @param params Consensus parameters
 * @param strErrorRet[out] Error message if validation fails
 * @return true if coinbase is valid, false otherwise
 */
bool CheckCoinbaseSubsidyAndPayees(const CTransaction& tx,
                                   const CBlockIndex* pindex,
                                   CAmount nTotalSubsidy,
                                   CAmount nFees,
                                   const Consensus::Params& params,
                                   std::string& strErrorRet);

#endif // BITCOIN_CONSENSUS_COINBASE_CHECKS_H
