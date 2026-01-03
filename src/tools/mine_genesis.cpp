// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

/**
 * Genesis block miner tool
 * 
 * Purpose: Mine a valid genesis block for a given network that matches
 * the exact parameters used in CreateGenesisBlock() in chainparams.cpp.
 * 
 * Usage: ./mine_genesis <network>
 *   where <network> is one of: regtest, testnet, mainnet
 * 
 * This tool recreates the genesis block with the same timestamp, output
 * script, and reward as specified in chainparams.cpp, then searches for
 * valid nTime/nNonce combinations that produce a block hash meeting the
 * difficulty target derived from nBits.
 */

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Wired 09/Jan/2014 The Grand Experiment Goes Live: Overstock.com Is Now Accepting Bitcoins";
    // P2WPKH output script for genesis premine
    const std::vector<unsigned char> genesisOutputBytes = ParseHex("0014697cd07c801b8bba094f759de4fe742a6bae0470");
    const CScript genesisOutputScript(genesisOutputBytes.begin(), genesisOutputBytes.end());
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void MineGenesis(const std::string& network)
{
    uint32_t nTime;
    uint32_t nBits;
    int32_t nVersion = 1;
    CAmount genesisReward = 9'000'000 * COIN;

    // Set parameters based on network
    if (network == "regtest") {
        nTime = 1767229261;
        nBits = 0x207fffff;
    } else if (network == "testnet") {
        nTime = 1767229261;
        nBits = 0x1e0ffff0;
    } else if (network == "mainnet") {
        nTime = 1767229261;
        nBits = 0x1e0ffff0;
    } else {
        std::cerr << "Error: Unknown network '" << network << "'\n";
        std::cerr << "Valid networks: regtest, testnet, mainnet\n";
        return;
    }

    std::cout << "Mining genesis block for " << network << "...\n";
    std::cout << "Initial nTime: " << nTime << "\n";
    std::cout << "nBits: 0x" << std::hex << nBits << std::dec << "\n\n";

    // Calculate target from nBits
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    std::cout << "Target: " << ArithToUint256(bnTarget).ToString() << "\n\n";

    // Start mining
    uint32_t nNonce = 0;
    uint64_t nAttempts = 0;
    const uint64_t reportInterval = 100000;

    while (true) {
        CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, nVersion, genesisReward);
        uint256 hash = genesis.GetHash();
        
        nAttempts++;
        
        if (UintToArith256(hash) <= bnTarget) {
            // Found valid genesis!
            std::cout << "\n=== FOUND VALID GENESIS ===\n\n";
            std::cout << "Copy these values to chainparams.cpp:\n\n";
            std::cout << "genesis = CreateGenesisBlock(" << nTime << ", " << nNonce << ", 0x" << std::hex << nBits << std::dec << ", " << nVersion << ", " << (genesisReward / COIN) << "'000'000 * COIN);\n";
            std::cout << "consensus.hashGenesisBlock = genesis.GetHash();\n";
            std::cout << "assert(consensus.hashGenesisBlock == uint256S(\"0x" << hash.ToString() << "\"));\n";
            std::cout << "assert(genesis.hashMerkleRoot == uint256S(\"0x" << genesis.hashMerkleRoot.ToString() << "\"));\n\n";
            std::cout << "Block details:\n";
            std::cout << "  Hash:        " << hash.ToString() << "\n";
            std::cout << "  MerkleRoot:  " << genesis.hashMerkleRoot.ToString() << "\n";
            std::cout << "  nTime:       " << nTime << "\n";
            std::cout << "  nNonce:      " << nNonce << "\n";
            std::cout << "  nBits:       0x" << std::hex << nBits << std::dec << "\n";
            std::cout << "  nVersion:    " << nVersion << "\n";
            std::cout << "  Total attempts: " << nAttempts << "\n";
            return;
        }
        
        if (nAttempts % reportInterval == 0) {
            std::cout << "Attempts: " << nAttempts << ", nNonce: " << nNonce << ", nTime: " << nTime << "\r" << std::flush;
        }
        
        nNonce++;
        
        // If we've exhausted all nonce values, increment time
        if (nNonce == 0) {
            nTime++;
            std::cout << "\nExhausted nonce range, incrementing nTime to " << nTime << "\n";
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <network>\n";
        std::cerr << "  where <network> is one of: regtest, testnet, mainnet\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " regtest\n";
        return 1;
    }

    std::string network = argv[1];
    MineGenesis(network);
    
    return 0;
}
