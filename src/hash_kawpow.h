// Copyright (c) 2024-2025 The NOSO Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HASH_KAWPOW_H
#define BITCOIN_HASH_KAWPOW_H

#include <crypto/sha3.h>
#include <span.h>
#include <uint256.h>

// Simplified KawPOW-style hash that reuses the Keccak core (SHA3-256). A full
// ProgPoW/DAG integration can be slotted in here in a follow-up once the
// supporting dataset generation is available.
template <typename T1>
inline uint256 HashKAWPOW(const T1 pbegin, const T1 pend)
{
    static const unsigned char pblank[1]{};
    uint256 result;

    const unsigned char* data;
    size_t size;
    if (pbegin == pend) {
        data = pblank;
        size = 0;
    } else {
        data = (const unsigned char*)&pbegin[0];
        size = (pend - pbegin) * sizeof(pbegin[0]);
    }

    SHA3_256()
        .Write({data, size})
        .Finalize(result);

    return result;
}

#endif // BITCOIN_HASH_KAWPOW_H
