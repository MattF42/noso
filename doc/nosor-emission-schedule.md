# NOSOR Genesis Blocks and Emission Schedule

## Overview

This document describes the NOSOR emission schedule, subsidy split, and genesis blocks for all networks.

## Emission Schedule

- **Maximum Supply**: 21,000,000 COIN (including 9M premine)
- **Initial Block Subsidy**: 12 COIN
- **Halving Interval**: 630,720 blocks
- **Halving Method**: Right shift (divide by 2) per halving period
- **Genesis Premine**: 9,000,000 COIN (assigned to DF P2WPKH)

### Subsidy Split

Each block's subsidy is deterministically split as follows:

- **Masternode**: 50% (floor rounding)
- **Miner**: 40% (floor rounding)
- **Dev Fund**: 1% (floor rounding)
- **Community Fund**: Remainder (ensures exact total)

The deterministic rounding ensures: `masternode + miner + devfee + community = total subsidy`

## Genesis Blocks

All networks use the same coinbase transaction structure:
- **Timestamp Message**: "Wired 09/Jan/2014 The Grand Experiment Goes Live: Overstock.com Is Now Accepting Bitcoins"
- **Genesis Reward**: 9,000,000 COIN
- **Recipient**: DF P2WPKH scriptPubKey `0014697cd07c801b8bba094f759de4fe742a6bae0470`
- **Merkle Root**: `f7ae7bdcbf014b839b1966d577100e54604827f42d1fd8ea47ba15b9057201df` (same for all networks)

### Mainnet
- **Time**: 1390095618
- **Nonce**: 531040
- **Bits**: 0x1e0ffff0
- **Genesis Hash**: `0000075f59dc8010ed51ded0e9129460cd3e0ce40e310d27f5480bf65faca280`

### Testnet
- **Time**: 1390666206
- **Nonce**: 3462550
- **Bits**: 0x1e0ffff0
- **Genesis Hash**: `00000171a41413144826a22b03ac9c4c589af3b2a80d77c1294c7778499f888b`

### Regtest / Devnet
- **Time**: 1417713337
- **Nonce**: 0
- **Bits**: 0x207fffff (very low difficulty)
- **Genesis Hash**: `1910ed0a59abddc238e91f51e2c5e02759b045ccc7bda662a16c9455c3a11386`

## DF P2WPKH Placeholder

The DF (Dev Fund) P2WPKH scriptPubKey is a placeholder address used for both the 1% dev fee and 9% community fee in the current implementation:

**scriptPubKey**: `0014697cd07c801b8bba094f759de4fe742a6bae0470`

This address controls the 9,000,000 COIN premine in the genesis block and receives the dev/community portions of each block subsidy.

**Note**: This is a placeholder address for testing purposes. Production deployment should use addresses with proper key management and governance.

## Testing

### Building

```bash
./autogen.sh
./configure --disable-wallet --disable-tests --disable-bench --without-gui
make -j$(nproc)
```

### Mining New Genesis Blocks

If you need to mine new genesis blocks (e.g., for a testnet reset):

```bash
# Mine testnet genesis
python3 contrib/devtools/mine-genesis.py --network=testnet

# Mine regtest genesis
python3 contrib/devtools/mine-genesis.py --network=regtest
```

The mining utility will output the correct parameters to update `src/chainparams.cpp`.

### Verifying Genesis Blocks

After building, verify the genesis blocks match:

```bash
# Run nosord with each network to verify genesis
./src/nosord -regtest -daemon
./src/nosor-cli -regtest getblockhash 0
./src/nosor-cli -regtest stop

# Verify it matches the documented genesis hash
```

## Implementation Files

- **Subsidy Logic**: `src/consensus/subsidy.h`, `src/consensus/subsidy.cpp`
- **Coinbase Validation**: `src/consensus/coinbase_checks.h`, `src/consensus/coinbase_checks.cpp`
- **Network Parameters**: `src/chainparams.cpp`
- **Genesis Mining Tool**: `contrib/devtools/mine-genesis.py`

## References

- Genesis block follows Bitcoin/Dash genesis structure
- Subsidy halving uses bit shift for efficiency
- Deterministic split uses floor rounding with remainder to community
- All values in satoshis (1 COIN = 100,000,000 satoshis)
