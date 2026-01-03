# Genesis Coinbase Premine is Now Spendable

## Summary

The genesis block's coinbase transaction is now treated as an ordinary, spendable coinbase output by default. This change ensures that the genesis premine is included in the chainstate/UTXO set and can be retrieved and spent like any other coinbase output.

## Consensus Impact

**⚠️ IMPORTANT: This is a consensus-impacting change.**

This change modifies how the genesis block is processed during chain validation. It **MUST** be applied before the mainnet or testnet genesis block is created and the network is launched. For any existing network with an immutable genesis block, this change would be a hard fork.

Since the NOSOR testnet and mainnet have not yet been launched, this change can be safely applied as part of the initial network configuration.

## Technical Changes

### Validation Layer
- **src/validation.cpp**: Removed special-case logic that previously skipped the genesis block's transactions during `ConnectBlock`. The genesis coinbase outputs are now added to the coins view (UTXO set) like any other coinbase transaction.

### RPC Layer  
- **src/rpc/rawtransaction.cpp**: Removed restrictions that prevented `getrawtransaction` and `getrawtransactionbatch` from retrieving the genesis coinbase transaction. The genesis transaction can now be queried via RPC like any other transaction.

### Coin Statistics Index
- **src/index/coinstatsindex.cpp**: Updated the coinstats index to process the genesis block outputs as spendable UTXOs rather than treating them as unspendable. The genesis block subsidy is no longer counted in `total_unspendables_genesis_block` when its outputs are in the UTXO set.

### Documentation
- **src/chainparams.cpp**: Updated comments to reflect that the genesis output is now spendable.

## Testing

A new functional test `feature_genesis_premine_spendable.py` has been added to verify:

1. `getrawtransaction` successfully returns the genesis coinbase transaction
2. `gettxout` successfully returns the genesis coinbase output  
3. The genesis UTXO is present in the chainstate after the genesis block is processed
4. The genesis UTXO persists correctly after reindex operations (`-reindex` and `-reindex-chainstate`)

## Impact on Existing Deployments

- **For new networks**: This change enables the intended behavior where the genesis premine is immediately spendable once mature (after 100 blocks for coinbase maturity).
- **For pre-existing networks**: This would be a consensus change requiring coordination. Since NOSOR networks have not launched, this does not apply.

## Usage Notes

- The genesis coinbase output becomes spendable after the standard coinbase maturity period (100 blocks).
- Wallet rescan will now detect and credit the genesis coinbase if the wallet owns the corresponding private key.
- The genesis transaction will appear in transaction indexes and can be queried via standard RPC calls.

## References

- Issue: Make genesis coinbase premine spendable by default
- Implementation: This PR implements the changes across validation, RPC, and testing layers
