// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Copyright (c) 2026 The NOSOR Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <llmq/params.h>
#include <util/ranges.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/underlying.h>
#include <versionbits.h>

#include <arith_uint256.h>

#include <assert.h>

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

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction is now treated as a normal spendable coinbase output.
 *
 * Genesis premine output script:
 * P2PKH: 76a914697cd07c801b8bba094f759de4fe742a6bae047088ac
 *
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // New genesis coinbase message
    const char* pszTimestamp = "NOSOR is NOSO-Rebirth Jan 2026";
    // P2WPKH output script for genesis premine (unchanged)
    const std::vector<unsigned char> genesisOutputBytes = ParseHex("76a914697cd07c801b8bba094f759de4fe742a6bae047088ac");
    const CScript genesisOutputScript(genesisOutputBytes.begin(), genesisOutputBytes.end());
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = gArgs.GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName, prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);
}

bool CChainParams::IsValidMNActivation(int nBit, int64_t timePast) const
{
    assert(nBit < VERSIONBITS_NUM_BITS);

    for (int index = 0; index < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++index) {
        if (consensus.vDeployments[index].bit == nBit) {
            auto& deployment = consensus.vDeployments[index];
            if (timePast > deployment.nTimeout || timePast < deployment.nStartTime) {
                LogPrintf("%s: activation by bit=%d deployment='%s' is out of time range start=%lld timeout=%lld\n", __func__, nBit, VersionBitsDeploymentInfo[Consensus::DeploymentPos(index)].name, deployment.nStartTime, deployment.nTimeout);
                continue;
            }
            if (!deployment.useEHF) {
                LogPrintf("%s: trying to set MnEHF for non-masternode activation fork bit=%d\n", __func__, nBit);
                return false;
            }
            LogPrintf("%s: set MnEHF for bit=%d is valid\n", __func__, nBit);
            return true;
        }
    }
    LogPrintf("%s: WARNING: unknown MnEHF fork bit=%d\n", __func__, nBit);
    return true;
}

void CChainParams::AddLLMQ(Consensus::LLMQType llmqType)
{
    assert(!GetLLMQ(llmqType).has_value());
    for (const auto& llmq_param : Consensus::available_llmqs) {
        if (llmq_param.type == llmqType) {
            consensus.llmqs.push_back(llmq_param);
            return;
        }
    }
    error("CChainParams::%s: unknown LLMQ type %d", __func__, static_cast<uint8_t>(llmqType));
    assert(false);
}

std::optional<Consensus::LLMQParams> CChainParams::GetLLMQ(Consensus::LLMQType llmqType) const
{
    for (const auto& llmq_param : consensus.llmqs) {
        if (llmq_param.type == llmqType) {
            return std::make_optional(llmq_param);
        }
    }
    return std::nullopt;
}

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 630720; // NOSOR: Halving every ~3 years
        consensus.nMasternodePaymentsStartBlock = 100000; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 158000; // actual historical value
        consensus.nMasternodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value
        consensus.nInstantSendConfirmationsRequired = 6;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 328008; // actual historical value
        consensus.nBudgetPaymentsCycleBlocks = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nSuperblockStartBlock = 614820; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
        consensus.nSuperblockStartHash = uint256S("0000000000020cb27c7ef164d21003d5d20cdca2f54dd9a9ca6d45f4d47f8aa3");
        consensus.nSuperblockCycle = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nSuperblockMaturityWindow = 1662; // ~(60*24*3)/2.6, ~3 days before actual Superblock is emitted
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.BIP34Height = 951;
        consensus.BIP34Hash = uint256S("0x000001f35e70f7c5705f64c6c5cc3dea9449e74d5b5c7cf74dad1bcca14a8012");
        consensus.BIP65Height = 619382; // 00000000000076d8fcea02ec0963de4abfd01e771fec0863f960c2c64fe6f357
        consensus.BIP66Height = 245817; // 00000000000b1fa2dfa312863570e13fae9ca7b5566cb27e55422620b469aefa
        consensus.BIP147Height = 939456; // 00000000000000117befca4fab5622514772f608852e5edd8df9c55464b6fe37
        consensus.CSVHeight = 622944; // 00000000000002e3d3a6224cfce80bae367fd3283d1e5a8ba50e5e60b2d2905d
        consensus.DIP0001Height = 782208; // 000000000000000cbc9cb551e8ee1ac7aa223585cbdfb755d3683bafd93679e4
        consensus.DIP0003Height = 1028160;
        consensus.DIP0003EnforcementHeight = 1047200;
        consensus.DIP0003EnforcementHash = uint256S("000000000000002d1734087b4c5afc3133e4e1c3e1a89218f62bcd9bb3d17f81");
        consensus.DIP0008Height = 1088640; // 00000000000000112e41e4b3afda8b233b8cc07c532d2eac5de097b68358c43e
        consensus.BRRHeight = 1374912; // 000000000000000c5a124f3eccfbe6e17876dca79cec9e63dfa70d269113c926
        consensus.DIP0020Height = 1516032; // 000000000000000f64ed3bd9af1078177ac026f6aa2677aa4d8beeae43be56cc
        consensus.DIP0024Height = 1737792; // 0000000000000001342be9c0b75ad40c276beaad91616423c4d9cb101b3db438
        consensus.DIP0024QuorumsHeight = 1738698; // 000000000000001aa25181e4c466e593992c98f9eb21c69ee757b8bb0af50244
        consensus.V19Height = 1899072; // 0000000000000015e32e73052d663626327004c81c5c22cb8b42c361015c0eae
        consensus.V20Height = 1987776; // 000000000000001bf41cff06b76780050682ca29e61a91c391893d4745579777
        consensus.MN_RRHeight = 2128896; // 0000000000000009a9696da93d3807eb14eb00a4ff449206d689156a21b27f26
        consensus.WithdrawalsHeight = 2201472; // 00000000000000210518749e17c00b035a2a4982c906236c28c41ea2231bf7ef
        consensus.MinBIP9WarningHeight = 2201472 + 2016; // withdrawals activation height + miner confirmation window
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 15200;
        consensus.nPowDGWHeight = 34140;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_V24].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE; // TODO
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT; // TODO
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nWindowSize = 4032;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdStart = 3226;     // 80% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdMin = 2420;       // 60% of 4032
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nFalloffCoeff = 5;          // this corresponds to 10 periods
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].useEHF = true;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256{};

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256{};

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xbf;
        pchMessageStart[1] = 0x0c;
        pchMessageStart[2] = 0x6b;
        pchMessageStart[3] = 0xbd;
        nDefaultPort = 20666;
        nDefaultPlatformP2PPort = 26656;
        nDefaultPlatformHTTPPort = 443;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 54;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1390095618, 531040, 0x1e0ffff0, 1, 9000000LL * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // assert(consensus.hashGenesisBlock == uint256S("0x0000075f59dc8010ed51ded0e9129460cd3e0ce40e310d27f5480bf65faca280"));
        // assert(genesis.hashMerkleRoot == uint256S("0xf7ae7bdcbf014b839b1966d577100e54604827f42d1fd8ea47ba15b9057201df"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("dnsseed.dash.org.");

        // Dash addresses start with 'X'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,76);
        // Dash script addresses start with '7'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,16);
        // Dash private keys start with '7' or 'X'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,204);
        // Dash BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        // Dash BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        // Dash BIP44 coin type is '5'
        nExtCoinType = 5;

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_400_60;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_60_75;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_100_67;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_400_85;

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = false;
        fAllowMultipleAddressesFromGroup = false;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 3;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        vSporkAddresses = {"Xgtyuk76vhuFW2iT7UAiHgNdWXCf3J34wh"};
        nMinSporkKeys = 1;
        consensus.strDevFeeScriptPubKey = "76a914697cd07c801b8bba094f759de4fe742a6bae047088ac"; // NosoR: DevFee scriptPubKey (P2PKH from pubkey)
        consensus.strCommunityFundScriptPubKey = "76a914697cd07c801b8bba094f759de4fe742a6bae047088ac"; // NosoR: Community fund scriptPubKey (mainnet uses same as devfee)

        nCreditPoolPeriodBlocks = 576;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
         // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
                0,
                0,
                0,
        };
    }
};

/**
 * Testnet (v3): public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 630720;
        consensus.nMasternodePaymentsStartBlock = 10; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 350;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256(); // do not check this on testnet
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
        consensus.nSuperblockMaturityWindow = 8;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1;
	consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1; // 0000039cf01242c7f921dcb4806a5994bc003b48c1973ae0c89b67809c2bb2ab
        consensus.BIP66Height = 1; // 0000002acdd29a14583540cb72e1c5cc83783560e38fa7081495d474fe1671f7
        consensus.BIP147Height = 1; // 0000000040c1480d413c9203664253ab18da284130c329bf88fcfc84312bcbe0
        consensus.CSVHeight = 1; // 00000005eb94d027e34649373669191188858a22c70f4a6d29105e559124cec7
        consensus.DIP0001Height = 1; // 00000001d60a01d8f1f39011cc6b26e3a1c97a24238cab856c2da71a4dd801a9
        consensus.DIP0003Height = 200;
        consensus.DIP0003EnforcementHeight = 500;
	consensus.DIP0003EnforcementHash = uint256();
        consensus.DIP0008Height = 1; // 000000000e9329d964d80e7dab2e704b43b6bd2b91fea1e9315d38932e55fb55
        consensus.BRRHeight = 1; // 0000001537dbfd09dea69f61c1f8b2afa27c8dc91c934e144797761c9f10367b
        consensus.DIP0020Height = 1; // 000000cf961868662fbfbb5d1af6f1caa1809f6a4e390efe5f8cd3031adea668
        consensus.DIP0024Height = 1; // 0000008d84e4efd890ae95c70a7a6126a70a80e5c19e4cb264a5b3469aeef172
        consensus.DIP0024QuorumsHeight = 1; // 0000003c43b3ae7fffe61278ca5537a0e256ebf4d709d45f0ab040271074d51e
        consensus.V19Height = 1; // 000004728b8ff2a16b9d4eebb0fd61eeffadc9c7fe4b0ec0b5a739869401ab5b
	consensus.V20Height = consensus.DIP0003Height; // V20 is important
	consensus.MN_RRHeight = 999999999; // MN_RR NOT USED ON NOSOR
        consensus.WithdrawalsHeight = 1148500; // 000000212a6fec2ee2af040c6d7a176360b154cbaa998888170cfd9ae7dd632d
        consensus.MinBIP9WarningHeight = 1148500 + 2016;  // withdrawals activation height + miner confirmation window
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 4002; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 4002; // TODO: make sure to drop all spork6 related code on next testnet reset
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_V24].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE; // TODO
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nWindowSize = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdStart = 80;       // 80% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdMin = 60;         // 60% of 100
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nFalloffCoeff = 5;          // this corresponds to 10 periods
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].useEHF = true;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256{};

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xff;
        nDefaultPort = 21666;
        nDefaultPlatformP2PPort = 22000;
        nDefaultPlatformHTTPPort = 22001;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 10;
        m_assumed_chain_state_size = 1;

	genesis = CreateGenesisBlock(1767229261, 1033047, 0x1e0ffff0, 1, 9000000LL * COIN);
	consensus.hashGenesisBlock = genesis.GetHash();
	assert(consensus.hashGenesisBlock == uint256S("0x00000dd15f836c24c3090a0c34d28cc2c7bbd479a445e90e8616ff11a4f355a3"));
	assert(genesis.hashMerkleRoot == uint256S("0x0328893ed72f12d1986bfd01c8ebc825884358f1a7839b8ba4558112bff572b4"));

        vFixedSeeds.clear();
        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));

        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // vSeeds.emplace_back("testnet-seed.dashdot.io."); // Just a static list of stable node(s), only supports x9
	vSeeds.emplace_back("38.242.252.153");
	vSeeds.emplace_back("4.233.61.8");
	vSeeds.emplace_back("testnet-seed.nosor.foztor.net");


        // Testnet Dash addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Testnet Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Testnet Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        AddLLMQ(Consensus::LLMQType::LLMQ_25_67);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_50_60;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_60_75;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_25_67;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_50_60;

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = false;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55"};
        nMinSporkKeys = 1;
        consensus.strDevFeeScriptPubKey = "76a914697cd07c801b8bba094f759de4fe742a6bae047088ac"; // NosoR: DevFee scriptPubKey (P2PKH from pubkey)
        consensus.strCommunityFundScriptPubKey = "0020c07d1212d177be88a4ead55ac708e592e5e6e0d63fec41f30c29c6fd34c7ff24"; // NosoR: Community fund P2WSH 2-of-3 multisig (testnet)

        nCreditPoolPeriodBlocks = 576;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
                0,
                0,
                0,
        };
    }
};

/**
 * Devnet: The Development network intended for developers use.
 */
class CDevNetParams : public CChainParams {
public:
    explicit CDevNetParams(const ArgsManager& args) {
        strNetworkID = CBaseChainParams::DEVNET;
        consensus.nSubsidyHalvingInterval = 630720;
        consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 4030;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
        consensus.nSuperblockMaturityWindow = 8;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1;   // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1;   // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1;   // BIP66 activated immediately on devnet
        consensus.BIP147Height = 1;  // BIP147 activated immediately on devnet
        consensus.CSVHeight = 1;     // BIP68 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.DIP0003Height = 2; // DIP0003 activated immediately on devnet
        consensus.DIP0003EnforcementHeight = 2; // DIP0003 activated immediately on devnet
        consensus.DIP0003EnforcementHash = uint256();
        consensus.DIP0008Height = 2; // DIP0008 activated immediately on devnet
        consensus.BRRHeight = 2;     // BRR (realloc) activated immediately on devnet
        consensus.DIP0020Height = 2; // DIP0020 activated immediately on devnet
        consensus.DIP0024Height = 2; // DIP0024 activated immediately on devnet
        consensus.DIP0024QuorumsHeight = 2; // DIP0024 activated immediately on devnet
        consensus.V19Height = 2;     // V19 activated immediately on devnet
        consensus.V20Height = 2;     // V20 activated immediately on devnet
        consensus.MN_RRHeight = 2;   // MN_RR activated immediately on devnet
        consensus.WithdrawalsHeight = 2;   // withdrawals activated immediately on devnet
        consensus.MinBIP9WarningHeight = 2 + 2016; // withdrawals activation height + miner confirmation window
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
        consensus.nPowDGWHeight = 4001;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_V24].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nStartTime = 1751328000;    // July 1, 2025
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nWindowSize = 120;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdStart = 96;       // 80% of 120
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdMin = 72;         // 60% of 120
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nFalloffCoeff = 5;          // this corresponds to 10 periods
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].useEHF = true;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        pchMessageStart[0] = 0xe2;
        pchMessageStart[1] = 0xca;
        pchMessageStart[2] = 0xff;
        pchMessageStart[3] = 0xce;
        nDefaultPort = 19799;
        nDefaultPlatformP2PPort = 22100;
        nDefaultPlatformHTTPPort = 22101;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateDevnetSubsidyAndDiffParametersFromArgs(args);
        genesis = CreateGenesisBlock(1767229261, 5, 0x207fffff, 1, 9000000LL * COIN);
	consensus.hashGenesisBlock = genesis.GetHash();
	// assert(consensus.hashGenesisBlock == uint256S("0x31525dcbfa6ebceba1293b99ef71f10eab6dd8762f60bb24296fef682d6ed8e7"));
	// assert(genesis.hashMerkleRoot == uint256S("0x96ecf82f38e933c4708db681a16190ccbff70592ec8e84229acc45839f506b21"));

        devnetGenesis = FindDevNetGenesisBlock(genesis, 9000000LL * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();
        //vSeeds.push_back(CDNSSeedData("dashevo.org.",  "devnet-seed.dashevo.org."));

        // Testnet Dash addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Testnet Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Testnet Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_50_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_60_75);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_60);
        AddLLMQ(Consensus::LLMQType::LLMQ_400_85);
        AddLLMQ(Consensus::LLMQType::LLMQ_100_67);
        AddLLMQ(Consensus::LLMQType::LLMQ_DEVNET);
        AddLLMQ(Consensus::LLMQType::LLMQ_DEVNET_DIP0024);
        AddLLMQ(Consensus::LLMQType::LLMQ_DEVNET_PLATFORM);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_DEVNET;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_DEVNET_DIP0024;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_DEVNET_PLATFORM;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_DEVNET;

        UpdateDevnetLLMQChainLocksFromArgs(args);
        UpdateDevnetLLMQInstantSendDIP0024FromArgs(args);
        UpdateDevnetLLMQPlatformFromArgs(args);
        UpdateDevnetLLMQMnhfFromArgs(args);
        UpdateLLMQDevnetParametersFromArgs(args);
        UpdateDevnetPowTargetSpacingFromArgs(args);

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = true;
        nLLMQConnectionRetryTimeout = 60;
        m_is_mockable_chain = false;

        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        vSporkAddresses = {"yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55"};
        nMinSporkKeys = 1;
        consensus.strDevFeeScriptPubKey = "76a914697cd07c801b8bba094f759de4fe742a6bae047088ac"; // NosoR: DevFee scriptPubKey (P2PKH from pubkey)
        consensus.strCommunityFundScriptPubKey = "0020c07d1212d177be88a4ead55ac708e592e5e6e0d63fec41f30c29c6fd34c7ff24"; // NosoR: Community fund P2WSH 2-of-3 multisig (devnet)

        nCreditPoolPeriodBlocks = 576;

        checkpointData = (CCheckpointData) {
            {
                {0, consensus.hashGenesisBlock},
                {1, devnetGenesis.GetHash()},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
    }

    /**
     * Allows modifying the subsidy and difficulty devnet parameters.
     */
    void UpdateDevnetSubsidyAndDiffParameters(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
    {
        consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
        consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
        consensus.nHighSubsidyFactor = nHighSubsidyFactor;
    }
    void UpdateDevnetSubsidyAndDiffParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the LLMQ type for ChainLocks.
     */
    void UpdateDevnetLLMQChainLocks(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeChainLocks = llmqType;
    }
    void UpdateDevnetLLMQChainLocksFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the LLMQ type for InstantSend (DIP0024).
     */
    void UpdateDevnetLLMQDIP0024InstantSend(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeDIP0024InstantSend = llmqType;
    }

    /**
     * Allows modifying the LLMQ type for Platform.
     */
    void UpdateDevnetLLMQPlatform(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypePlatform = llmqType;
    }

    /**
     * Allows modifying the LLMQ type for Mnhf.
     */
    void UpdateDevnetLLMQMnhf(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeMnhf = llmqType;
    }

    /**
     * Allows modifying PowTargetSpacing
     */
    void UpdateDevnetPowTargetSpacing(int64_t nPowTargetSpacing)
    {
        consensus.nPowTargetSpacing = nPowTargetSpacing;
    }

    /**
     * Allows modifying parameters of the devnet LLMQ
     */
    void UpdateLLMQDevnetParameters(int size, int threshold)
    {
        auto params = ranges::find_if(consensus.llmqs, [](const auto& llmq){ return llmq.type == Consensus::LLMQType::LLMQ_DEVNET;});
        assert(params != consensus.llmqs.end());
        params->size = size;
        params->minSize = threshold;
        params->threshold = threshold;
        params->dkgBadVotesThreshold = threshold;
    }
    void UpdateLLMQDevnetParametersFromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQInstantSendFromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQInstantSendDIP0024FromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQPlatformFromArgs(const ArgsManager& args);
    void UpdateDevnetLLMQMnhfFromArgs(const ArgsManager& args);
    void UpdateDevnetPowTargetSpacingFromArgs(const ArgsManager& args);
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 630720; // NOSOR: halving every 630,720 blocks (use same as mainnet for consistency)
        consensus.nMasternodePaymentsStartBlock = 10;
        consensus.nMasternodePaymentsIncreaseBlock = 350;
        consensus.nMasternodePaymentsIncreasePeriod = 10;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 1000;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 1500;
        consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
        consensus.nSuperblockCycle = 20;
        consensus.nSuperblockMaturityWindow = 10;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1;   // Always active unless overridden
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;   // Always active unless overridden
        consensus.BIP66Height = 1;   // Always active unless overridden
        consensus.BIP147Height = 0;  // Always active unless overridden
        consensus.CSVHeight = 1;     // Always active unless overridden
        consensus.DIP0001Height = 1; // Always active unless overridden
        consensus.DIP0003Height = 200; // Always active for DashTestFramework in functional tests (see dip3params)
                                       // For unit tests and for BitcoinTestFramework is disabled due to missing quorum commitment for blocks created by helpers such as create_blocks
        consensus.DIP0003EnforcementHeight = 500;
        consensus.DIP0003EnforcementHash = uint256();
        consensus.DIP0008Height = 1; // Always active unless overridden
        consensus.BRRHeight = 1;     // Always active unless overridden
        consensus.DIP0020Height = 1; // Always active unless overridden
        consensus.DIP0024Height = 1; // Always have dip0024 quorums unless overridden
        consensus.DIP0024QuorumsHeight = 1; // Always have dip0024 quorums unless overridden
        consensus.V19Height = 1; // Always active unless overridden
        consensus.V20Height = consensus.DIP0003Height; // V20 is important
        consensus.MN_RRHeight = 999999999; // MN_RR NOT USED ON NOSOR
        consensus.WithdrawalsHeight = 600;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        consensus.nPowTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nPowKGWHeight = 15200; // same as mainnet
        consensus.nPowDGWHeight = 34140; // same as mainnet
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_V24].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nWindowSize = 250;
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdStart = 250 / 5 * 4;     // 80% of window size
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nThresholdMin = 250 / 5 * 3;       // 60% of window size
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].nFalloffCoeff = 5;                 // this corresponds to 10 periods
        consensus.vDeployments[Consensus::DEPLOYMENT_V24].useEHF = true;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 22666;
        nDefaultPlatformP2PPort = 22200;
        nDefaultPlatformHTTPPort = 22201;
        nPruneAfterHeight = args.GetBoolArg("-fastprune", false) ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);
        UpdateDIP3ParametersFromArgs(args);
        UpdateBudgetParametersFromArgs(args);

        
        genesis = CreateGenesisBlock(1767229261, 0, 0x207fffff, 1, 9000000LL * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x5162fec28a8dfc55130d112632fea689332f77f4702dce4d15686a9c269e4360"));
        assert(genesis.hashMerkleRoot == uint256S("0x0328893ed72f12d1986bfd01c8ebc825884358f1a7839b8ba4558112bff572b4"));



        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();
        vSeeds.emplace_back("dummySeed.invalid.");

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        fRequireRoutableExternalIP = false;
        m_is_test_chain = true;
        fAllowMultipleAddressesFromGroup = true;
        nLLMQConnectionRetryTimeout = 1; // must be lower then the LLMQ signing session timeout so that tests have control over failing behavior
        m_is_mockable_chain = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        nPoolMinParticipants = 2;
        nPoolMaxParticipants = 20;

        // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
        vSporkAddresses = {"yj949n1UH6fDhw6HtVE5VMj2iSTaSWBMcW"};
        nMinSporkKeys = 1;
        consensus.strDevFeeScriptPubKey = "76a914697cd07c801b8bba094f759de4fe742a6bae047088ac"; // NosoR: DevFee scriptPubKey (P2PKH from pubkey)
        consensus.strCommunityFundScriptPubKey = "0020c07d1212d177be88a4ead55ac708e592e5e6e0d63fec41f30c29c6fd34c7ff24"; // NosoR: Community fund P2WSH 2-of-3 multisig (regtest)

        nCreditPoolPeriodBlocks = 100;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{};

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Regtest Dash addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Regtest Dash script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Regtest Dash BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        // Regtest Dash BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        // Regtest Dash BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        // long living quorum params
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_INSTANTSEND);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_V17);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_DIP0024);
        AddLLMQ(Consensus::LLMQType::LLMQ_TEST_PLATFORM);
        consensus.llmqTypeChainLocks = Consensus::LLMQType::LLMQ_TEST;
        consensus.llmqTypeDIP0024InstantSend = Consensus::LLMQType::LLMQ_TEST_DIP0024;
        consensus.llmqTypePlatform = Consensus::LLMQType::LLMQ_TEST_PLATFORM;
        consensus.llmqTypeMnhf = Consensus::LLMQType::LLMQ_TEST;

        UpdateLLMQTestParametersFromArgs(args, Consensus::LLMQType::LLMQ_TEST);
        UpdateLLMQTestParametersFromArgs(args, Consensus::LLMQType::LLMQ_TEST_INSTANTSEND);
        UpdateLLMQTestParametersFromArgs(args, Consensus::LLMQType::LLMQ_TEST_PLATFORM);
        UpdateLLMQInstantSendDIP0024FromArgs(args);
        // V20 features for CbTx (credit pool, CL) have no meaning without masternodes
        assert(consensus.V20Height >= consensus.DIP0003Height);
        // MN_RR reallocate part of reward to CreditPool which exits since V20
        assert(consensus.MN_RRHeight >= consensus.V20Height);
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height, int64_t nWindowSize, int64_t nThresholdStart, int64_t nThresholdMin, int64_t nFalloffCoeff, int64_t nUseEHF)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
        if (nWindowSize != -1) {
            consensus.vDeployments[d].nWindowSize = nWindowSize;
        }
        if (nThresholdStart != -1) {
            consensus.vDeployments[d].nThresholdStart = nThresholdStart;
        }
        if (nThresholdMin != -1) {
            consensus.vDeployments[d].nThresholdMin = nThresholdMin;
        }
        if (nFalloffCoeff != -1) {
            consensus.vDeployments[d].nFalloffCoeff = nFalloffCoeff;
        }
        if (nUseEHF != -1) {
            consensus.vDeployments[d].useEHF = nUseEHF > 0;
        }
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the DIP3 activation and enforcement height
     */
    void UpdateDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
    {
        consensus.DIP0003Height = nActivationHeight;
        consensus.DIP0003EnforcementHeight = nEnforcementHeight;
    }
    void UpdateDIP3ParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying the budget regtest parameters.
     */
    void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
    {
        consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
        consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
        consensus.nSuperblockStartBlock = nSuperblockStartBlock;
    }
    void UpdateBudgetParametersFromArgs(const ArgsManager& args);

    /**
     * Allows modifying parameters of the test LLMQ
     */
    void UpdateLLMQTestParameters(int size, int threshold, const Consensus::LLMQType llmqType)
    {
        auto params = ranges::find_if(consensus.llmqs, [llmqType](const auto& llmq){ return llmq.type == llmqType;});
        assert(params != consensus.llmqs.end());
        params->size = size;
        params->minSize = threshold;
        params->threshold = threshold;
        params->dkgBadVotesThreshold = threshold;
    }

    /**
     * Allows modifying the LLMQ type for InstantSend (DIP0024).
     */
    void UpdateLLMQDIP0024InstantSend(Consensus::LLMQType llmqType)
    {
        consensus.llmqTypeDIP0024InstantSend = llmqType;
    }

    void UpdateLLMQTestParametersFromArgs(const ArgsManager& args, const Consensus::LLMQType llmqType);
    void UpdateLLMQInstantSendDIP0024FromArgs(const ArgsManager& args);
};

static void MaybeUpdateHeights(const ArgsManager& args, Consensus::Params& consensus)
{
    for (const std::string& arg : args.GetArgs("-testactivationheight")) {
        const auto found{arg.find('@')};
        if (found == std::string::npos) {
            throw std::runtime_error(strprintf("Invalid format (%s) for -testactivationheight=name@height.", arg));
        }
        const auto name{arg.substr(0, found)};
        const auto value{arg.substr(found + 1)};
        int32_t height;
        if (!ParseInt32(value, &height) || height < 0 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Invalid height value (%s) for -testactivationheight=name@height.", arg));
        }
        if (name == "bip147") {
            consensus.BIP147Height = int{height};
        } else if (name == "bip34") {
            consensus.BIP34Height = int{height};
        } else if (name == "dersig") {
            consensus.BIP66Height = int{height};
        } else if (name == "cltv") {
            consensus.BIP65Height = int{height};
        } else if (name == "csv") {
            consensus.CSVHeight = int{height};
        } else if (name == "brr") {
            consensus.BRRHeight = int{height};
        } else if (name == "dip0001") {
            consensus.DIP0001Height = int{height};
        } else if (name == "dip0008") {
            consensus.DIP0008Height = int{height};
        } else if (name == "dip0024") {
            consensus.DIP0024Height = int{height};
            consensus.DIP0024QuorumsHeight = int{height};
        } else if (name == "v19") {
            consensus.V19Height = int{height};
        } else if (name == "v20") {
            consensus.V20Height = int{height};
        } else if (name == "mn_rr") {
            consensus.MN_RRHeight = int{height};
        } else {
            throw std::runtime_error(strprintf("Invalid name (%s) for -testactivationheight=name@height.", arg));
        }
    }
}

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    MaybeUpdateHeights(args, consensus);

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams = SplitString(strDeployment, ':');
        if (vDeploymentParams.size() != 3 && vDeploymentParams.size() != 4 && vDeploymentParams.size() != 6 && vDeploymentParams.size() != 9) {
            throw std::runtime_error("Version bits parameters malformed, expecting "
                    "<deployment>:<start>:<end> or "
                    "<deployment>:<start>:<end>:<min_activation_height> or "
                    "<deployment>:<start>:<end>:<min_activation_height>:<window>:<threshold> or "
                    "<deployment>:<start>:<end>:<min_activation_height>:<window>:<thresholdstart>:<thresholdmin>:<falloffcoeff>:<useehf>");
        }
        int64_t nStartTime, nTimeout, nWindowSize = -1, nThresholdStart = -1, nThresholdMin = -1, nFalloffCoeff = -1, nUseEHF = -1;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        if (vDeploymentParams.size() >= 6) {
            if (!ParseInt64(vDeploymentParams[4], &nWindowSize)) {
                throw std::runtime_error(strprintf("Invalid nWindowSize (%s)", vDeploymentParams[4]));
            }
            if (!ParseInt64(vDeploymentParams[5], &nThresholdStart)) {
                throw std::runtime_error(strprintf("Invalid nThresholdStart (%s)", vDeploymentParams[5]));
            }
        }
        if (vDeploymentParams.size() == 9) {
            if (!ParseInt64(vDeploymentParams[6], &nThresholdMin)) {
                throw std::runtime_error(strprintf("Invalid nThresholdMin (%s)", vDeploymentParams[6]));
            }
            if (!ParseInt64(vDeploymentParams[7], &nFalloffCoeff)) {
                throw std::runtime_error(strprintf("Invalid nFalloffCoeff (%s)", vDeploymentParams[7]));
            }
            if (!ParseInt64(vDeploymentParams[8], &nUseEHF)) {
                throw std::runtime_error(strprintf("Invalid nUseEHF (%s)", vDeploymentParams[8]));
            }
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff, nUseEHF);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%ld, window=%ld, thresholdstart=%ld, thresholdmin=%ld, falloffcoeff=%ld, useehf=%ld\n",
                          vDeploymentParams[0], nStartTime, nTimeout, min_activation_height, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff, nUseEHF);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

void CRegTestParams::UpdateDIP3ParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-dip3params")) return;

    std::string strParams = args.GetArg("-dip3params", "");
    std::vector<std::string> vParams = SplitString(strParams, ':');
    if (vParams.size() != 2) {
        throw std::runtime_error("DIP3 parameters malformed, expecting <activation>:<enforcement>");
    }
    int nDIP3ActivationHeight, nDIP3EnforcementHeight;
    if (!ParseInt32(vParams[0], &nDIP3ActivationHeight)) {
        throw std::runtime_error(strprintf("Invalid activation height (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &nDIP3EnforcementHeight)) {
        throw std::runtime_error(strprintf("Invalid enforcement height (%s)", vParams[1]));
    }
    LogPrintf("Setting DIP3 parameters to activation=%ld, enforcement=%ld\n", nDIP3ActivationHeight, nDIP3EnforcementHeight);
    UpdateDIP3Parameters(nDIP3ActivationHeight, nDIP3EnforcementHeight);
}

void CRegTestParams::UpdateBudgetParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-budgetparams")) return;

    std::string strParams = args.GetArg("-budgetparams", "");
    std::vector<std::string> vParams = SplitString(strParams, ':');
    if (vParams.size() != 3) {
        throw std::runtime_error("Budget parameters malformed, expecting <masternode>:<budget>:<superblock>");
    }
    int nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock;
    if (!ParseInt32(vParams[0], &nMasternodePaymentsStartBlock)) {
        throw std::runtime_error(strprintf("Invalid masternode start height (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &nBudgetPaymentsStartBlock)) {
        throw std::runtime_error(strprintf("Invalid budget start block (%s)", vParams[1]));
    }
    if (!ParseInt32(vParams[2], &nSuperblockStartBlock)) {
        throw std::runtime_error(strprintf("Invalid superblock start height (%s)", vParams[2]));
    }
    LogPrintf("Setting budget parameters to masternode=%ld, budget=%ld, superblock=%ld\n", nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
    UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void CRegTestParams::UpdateLLMQTestParametersFromArgs(const ArgsManager& args, const Consensus::LLMQType llmqType)
{
    assert(llmqType == Consensus::LLMQType::LLMQ_TEST || llmqType == Consensus::LLMQType::LLMQ_TEST_INSTANTSEND || llmqType == Consensus::LLMQType::LLMQ_TEST_PLATFORM);

    std::string cmd_param{"-llmqtestparams"}, llmq_name{"LLMQ_TEST"};
    if (llmqType == Consensus::LLMQType::LLMQ_TEST_INSTANTSEND) {
        cmd_param = "-llmqtestinstantsendparams";
        llmq_name = "LLMQ_TEST_INSTANTSEND";
    }
    if (llmqType == Consensus::LLMQType::LLMQ_TEST_PLATFORM) {
        cmd_param = "-llmqtestplatformparams";
        llmq_name = "LLMQ_TEST_PLATFORM";
    }

    if (!args.IsArgSet(cmd_param)) return;

    std::string strParams = args.GetArg(cmd_param, "");
    std::vector<std::string> vParams = SplitString(strParams, ':');
    if (vParams.size() != 2) {
        throw std::runtime_error(strprintf("%s parameters malformed, expecting <size>:<threshold>", llmq_name));
    }
    int size, threshold;
    if (!ParseInt32(vParams[0], &size)) {
        throw std::runtime_error(strprintf("Invalid %s size (%s)", llmq_name, vParams[0]));
    }
    if (!ParseInt32(vParams[1], &threshold)) {
        throw std::runtime_error(strprintf("Invalid %s threshold (%s)", llmq_name, vParams[1]));
    }
    LogPrintf("Setting %s parameters to size=%ld, threshold=%ld\n", llmq_name, size, threshold);
    UpdateLLMQTestParameters(size, threshold, llmqType);
}

void CRegTestParams::UpdateLLMQInstantSendDIP0024FromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqtestinstantsenddip0024")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeDIP0024InstantSend);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqtestinstantsenddip0024", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqtestinstantsenddip0024.");
    }
    LogPrintf("Setting llmqtestinstantsenddip0024 to %ld\n", ToUnderlying(llmqType));
    UpdateLLMQDIP0024InstantSend(llmqType);
}

void CDevNetParams::UpdateDevnetSubsidyAndDiffParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-minimumdifficultyblocks") && !args.IsArgSet("-highsubsidyblocks") && !args.IsArgSet("-highsubsidyfactor")) return;

    int nMinimumDifficultyBlocks = gArgs.GetIntArg("-minimumdifficultyblocks", consensus.nMinimumDifficultyBlocks);
    int nHighSubsidyBlocks = gArgs.GetIntArg("-highsubsidyblocks", consensus.nHighSubsidyBlocks);
    int nHighSubsidyFactor = gArgs.GetIntArg("-highsubsidyfactor", consensus.nHighSubsidyFactor);
    LogPrintf("Setting minimumdifficultyblocks=%ld, highsubsidyblocks=%ld, highsubsidyfactor=%ld\n", nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
    UpdateDevnetSubsidyAndDiffParameters(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}

void CDevNetParams::UpdateDevnetLLMQChainLocksFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqchainlocks")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeChainLocks);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqchainlocks", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            if (params.useRotation) {
                throw std::runtime_error("LLMQ type specified for -llmqchainlocks must NOT use rotation");
            }
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqchainlocks.");
    }
    LogPrintf("Setting llmqchainlocks to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQChainLocks(llmqType);
}

void CDevNetParams::UpdateDevnetLLMQInstantSendDIP0024FromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqinstantsenddip0024")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeDIP0024InstantSend);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqinstantsenddip0024", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            if (!params.useRotation) {
                throw std::runtime_error("LLMQ type specified for -llmqinstantsenddip0024 must use rotation");
            }
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqinstantsenddip0024.");
    }
    LogPrintf("Setting llmqinstantsenddip0024 to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQDIP0024InstantSend(llmqType);
}

void CDevNetParams::UpdateDevnetLLMQPlatformFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqplatform")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypePlatform);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqplatform", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqplatform.");
    }
    LogPrintf("Setting llmqplatform to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQPlatform(llmqType);
}

void CDevNetParams::UpdateDevnetLLMQMnhfFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqmnhf")) return;

    const auto& llmq_params_opt = GetLLMQ(consensus.llmqTypeMnhf);
    assert(llmq_params_opt.has_value());

    std::string strLLMQType = gArgs.GetArg("-llmqmnhf", std::string(llmq_params_opt->name));

    Consensus::LLMQType llmqType = Consensus::LLMQType::LLMQ_NONE;
    for (const auto& params : consensus.llmqs) {
        if (params.name == strLLMQType) {
            llmqType = params.type;
        }
    }
    if (llmqType == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Invalid LLMQ type specified for -llmqmnhf.");
    }
    LogPrintf("Setting llmqmnhf to size=%ld\n", static_cast<uint8_t>(llmqType));
    UpdateDevnetLLMQMnhf(llmqType);
}

void CDevNetParams::UpdateDevnetPowTargetSpacingFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-powtargetspacing")) return;

    std::string strPowTargetSpacing = gArgs.GetArg("-powtargetspacing", "");

    int64_t powTargetSpacing;
    if (!ParseInt64(strPowTargetSpacing, &powTargetSpacing)) {
        throw std::runtime_error(strprintf("Invalid parsing of powTargetSpacing (%s)", strPowTargetSpacing));
    }

    if (powTargetSpacing < 1) {
        throw std::runtime_error(strprintf("Invalid value of powTargetSpacing (%s)", strPowTargetSpacing));
    }

    LogPrintf("Setting powTargetSpacing to %ld\n", powTargetSpacing);
    UpdateDevnetPowTargetSpacing(powTargetSpacing);
}

void CDevNetParams::UpdateLLMQDevnetParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-llmqdevnetparams")) return;

    std::string strParams = args.GetArg("-llmqdevnetparams", "");
    std::vector<std::string> vParams = SplitString(strParams, ':');
    if (vParams.size() != 2) {
        throw std::runtime_error("LLMQ_DEVNET parameters malformed, expecting <size>:<threshold>");
    }
    int size, threshold;
    if (!ParseInt32(vParams[0], &size)) {
        throw std::runtime_error(strprintf("Invalid LLMQ_DEVNET size (%s)", vParams[0]));
    }
    if (!ParseInt32(vParams[1], &threshold)) {
        throw std::runtime_error(strprintf("Invalid LLMQ_DEVNET threshold (%s)", vParams[1]));
    }
    LogPrintf("Setting LLMQ_DEVNET parameters to size=%ld, threshold=%ld\n", size, threshold);
    UpdateLLMQDevnetParameters(size, threshold);
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::DEVNET) {
        return std::unique_ptr<CChainParams>(new CDevNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}

void SetupChainParamsOptions(ArgsManager& argsman)
{
    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("-budgetparams=<masternode>:<budget>:<superblock>", "Override masternode, budget and superblock start heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-dip3params=<activation>:<enforcement>", "Override DIP3 activation and enforcement heights (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-highsubsidyblocks=<n>", "The number of blocks with a higher than normal subsidy to mine at the start of a chain. Block after that height will have fixed subsidy base. (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-highsubsidyfactor=<n>", "The factor to multiply the normal block subsidy by while in the highsubsidyblocks window of a chain (default: 1, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqchainlocks=<quorum name>", "Override the default LLMQ type used for ChainLocks. Allows using ChainLocks with smaller LLMQs. (default: llmq_devnet, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqdevnetparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_DEVNET quorum (devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqinstantsenddip0024=<quorum name>", "Override the default LLMQ type used for InstantSendDIP0024. (default: llmq_devnet_dip0024, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqplatform=<quorum name>", "Override the default LLMQ type used for Platform. (default: llmq_devnet_platform, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqmnhf=<quorum name>", "Override the default LLMQ type used for EHF. (default: llmq_devnet, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestinstantsenddip0024=<quorum name>", "Override the default LLMQ type used for InstantSendDIP0024. Used mainly to test Platform. (default: llmq_test_dip0024, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestinstantsendparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST_INSTANTSEND quorums (default: 3:2, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST quorum (default: 3:2, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-llmqtestplatformparams=<size>:<threshold>", "Override the default LLMQ size for the LLMQ_TEST_PLATFORM quorum (default: 3:2, regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-minimumdifficultyblocks=<n>", "The number of blocks that can be mined with the minimum difficulty at the start of a chain (default: 0, devnet-only)", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-powtargetspacing=<n>", "Override the default PowTargetSpacing value in seconds (default: 2.5 minutes, devnet-only)", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-testactivationheight=name@height.", "Set the activation height of 'name' (bip147, bip34, dersig, cltv, csv, brr, dip0001, dip0008, dip0024, v19, v20, mn_rr). (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    argsman.AddArg("-vbparams=<deployment>:<start>:<end>(:min_activation_height(:<window>:<threshold/thresholdstart>(:<thresholdmin>:<falloffcoeff>:<mnactivation>)))",
                 "Use given start/end times and min_activation_height for specified version bits deployment (regtest-only). "
                 "Specifying window, threshold/thresholdstart, thresholdmin, falloffcoeff and mnactivation is optional.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
}
