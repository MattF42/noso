#!/usr/bin/env python3
# Copyright (c) 2026 The NOSOR Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Genesis block mining utility for NOSOR.

This script helps mine genesis blocks for testnet and regtest networks
with the correct premine structure.

Usage:
    ./mine-genesis.py --network=testnet
    ./mine-genesis.py --network=regtest
"""

import argparse
import hashlib
import struct
import time

def double_sha256(data):
    """Compute double SHA256 hash."""
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def target_from_bits(bits):
    """Convert compact bits representation to target."""
    exp = bits >> 24
    mant = bits & 0xffffff
    if mant & 0x800000:
        mant >>= 8
        exp -= 1
    target = mant * (1 << (8 * (exp - 3)))
    return target

def serialize_block_header(version, prev_block, merkle_root, timestamp, bits, nonce):
    """Serialize block header for hashing."""
    header = struct.pack('<I', version)
    header += bytes.fromhex(prev_block)[::-1]
    header += bytes.fromhex(merkle_root)[::-1]
    header += struct.pack('<I', timestamp)
    header += struct.pack('<I', bits)
    header += struct.pack('<I', nonce)
    return header

def hash_block_header(header):
    """Hash block header using double SHA256."""
    return double_sha256(header)

def mine_genesis(network):
    """Mine genesis block for specified network."""
    
    # Common parameters
    version = 1
    prev_block = '0' * 64  # Genesis has no previous block
    timestamp_str = "Wired 09/Jan/2014 The Grand Experiment Goes Live: Overstock.com Is Now Accepting Bitcoins"
    
    # Premine output: 9,000,000 COIN to DF P2WPKH
    # scriptPubKey: 0014697cd07c801b8bba094f759de4fe742a6bae0470
    premine = 9_000_000 * 100_000_000  # 9M COIN in satoshis
    
    # This merkle root should match the one calculated by CreateGenesisBlock
    # The merkle root is the same for all networks since the coinbase tx is identical
    merkle_root = "f7ae7bdcbf014b839b1966d577100e54604827f42d1fd8ea47ba15b9057201df"
    
    if network == 'testnet':
        # Testnet parameters
        timestamp = 1390666206  # Different from mainnet
        bits = 0x1e0ffff0
        print(f"Mining testnet genesis block...")
        print(f"Timestamp: {timestamp}")
        print(f"Bits: 0x{bits:08x}")
    elif network == 'regtest':
        # Regtest parameters  
        timestamp = 1417713337  # Different from mainnet/testnet
        bits = 0x207fffff  # Very easy difficulty for regtest
        print(f"Mining regtest genesis block...")
        print(f"Timestamp: {timestamp}")
        print(f"Bits: 0x{bits:08x}")
    else:
        print(f"Unknown network: {network}")
        return
    
    print(f"Merkle root: {merkle_root}")
    print(f"Mining...")
    
    target = target_from_bits(bits)
    start_time = time.time()
    
    for nonce in range(0, 0xFFFFFFFF):
        header = serialize_block_header(version, prev_block, merkle_root, timestamp, bits, nonce)
        hash_result = hash_block_header(header)
        hash_int = int.from_bytes(hash_result[::-1], byteorder='big')
        
        if nonce % 100000 == 0:
            elapsed = time.time() - start_time
            rate = nonce / elapsed if elapsed > 0 else 0
            print(f"Nonce: {nonce:10d}, Rate: {rate:10.0f} H/s", end='\r')
        
        if hash_int <= target:
            hash_hex = hash_result[::-1].hex()
            print(f"\n\nGenesis block found!")
            print(f"Nonce: {nonce}")
            print(f"Hash: {hash_hex}")
            print(f"Time: {timestamp}")
            print(f"Merkle root: {merkle_root}")
            print(f"\nUpdate chainparams.cpp:")
            print(f"genesis = CreateGenesisBlock({timestamp}, {nonce}, 0x{bits:08x}, 1, 9'000'000 * COIN);")
            print(f"consensus.hashGenesisBlock = genesis.GetHash();")
            print(f"assert(consensus.hashGenesisBlock == uint256S(\"0x{hash_hex}\"));")
            print(f"assert(genesis.hashMerkleRoot == uint256S(\"{merkle_root}\"));")
            return
    
    print("\nFailed to find valid nonce in range [0, 0xFFFFFFFF]")

def main():
    parser = argparse.ArgumentParser(description='Mine NOSOR genesis blocks')
    parser.add_argument('--network', required=True, choices=['testnet', 'regtest'],
                        help='Network to mine genesis for (testnet or regtest)')
    args = parser.parse_args()
    
    mine_genesis(args.network)

if __name__ == '__main__':
    main()
