// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/raven-config.h"
#endif

#include "base58.h"
#include "clientversion.h"
#include "consensus/consensus.h"
#include "core_io.h"
#include "policy/policy.h"
#include "policy/rbf.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include <univalue.h>
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"

#include <stdio.h>

#include <boost/algorithm/string.hpp>

static bool fCreateBlank;
static const int CONTINUE_EXECUTION=-1;

//
// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
//
static int AppInitRawTx(int argc, char* argv[])
{
    //
    // Parameters
    //
    gArgs.ParseParameters(argc, argv);

    // Check for -testnet or -regtest parameter (GetParams() calls are only valid after this clause)
    try {
        SelectParams(ChainNameFromCommandLine());
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }

    fCreateBlank = gArgs.GetBoolArg("-create", false);

    if (argc<2 || gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") || gArgs.IsArgSet("-help"))
    {
        // First part of help message is specific to this utility
        std::string strUsage = strprintf(_("%s raven-tx utility version"), _(PACKAGE_NAME)) + " " + FormatFullVersion() + "\n\n" +
            _("Usage:") + "\n" +
              "  raven-tx [options] <hex-tx> [commands]  " + _("Update hex-encoded raven transaction") + "\n" +
              "  raven-tx [options] -create [commands]   " + _("Create hex-encoded raven transaction") + "\n" +
              "\n";

        fprintf(stdout, "%s", strUsage.c_str());

        strUsage = HelpMessageGroup(_("Options:"));
        strUsage += HelpMessageOpt("-?", _("This help message"));
        strUsage += HelpMessageOpt("-create", _("Create new, empty TX."));
        strUsage += HelpMessageOpt("-txid", _("Output only the hex-encoded transaction id of the resultant transaction."));

        fprintf(stdout, "%s", strUsage.c_str());

        strUsage = HelpMessageGroup(_("Commands:"));
        strUsage += HelpMessageOpt("delin=N", _("Delete input N from TX"));
        strUsage += HelpMessageOpt("delout=N", _("Delete output N from TX"));
        strUsage += HelpMessageOpt("in=TXID:VOUT(:SEQUENCE_NUMBER)", _("Add input to TX"));
        strUsage += HelpMessageOpt("locktime=N", _("Set TX lock time to N"));
        strUsage += HelpMessageOpt("nversion=N", _("Set TX version to N"));
        strUsage += HelpMessageOpt("replaceable(=N)", _("Set RBF opt-in sequence number for input N (if not provided, opt-in all available inputs)"));
        strUsage += HelpMessageOpt("outaddr=VALUE:ADDRESS", _("Add address-based output to TX"));
        strUsage += HelpMessageOpt("outpubkey=VALUE:PUBKEY[:FLAGS]", _("Add pay-to-pubkey output to TX") + ". " +
            _("Optionally add the \"W\" flag to produce a pay-to-witness-pubkey-hash output") + ". " +
            _("Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash."));
        strUsage += HelpMessageOpt("outdata=[VALUE:]DATA", _("Add data-based output to TX"));
        strUsage += HelpMessageOpt("outscript=VALUE:SCRIPT[:FLAGS]", _("Add raw script output to TX") + ". " +
            _("Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output") + ". " +
            _("Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash."));
        strUsage += HelpMessageOpt("outmultisig=VALUE:REQUIRED:PUBKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]", _("Add Pay To n-of-m Multi-sig output to TX. n = REQUIRED, m = PUBKEYS") + ". " +
            _("Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output") + ". " +
            _("Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash."));
        fprintf(stdout, "%s", strUsage.c_str());

        if (argc < 2) {
            fprintf(stderr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    return CONTINUE_EXECUTION;
}

static CAmount ExtractAndValidateValue(const std::string& strValue)
{
    CAmount value;
    if (!ParseMoney(strValue, value))
        throw std::runtime_error("invalid TX output value");
    return value;
}

static void MutateTxVersion(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newVersion = atoi64(cmdVal);
    if (newVersion < 1 || newVersion > CTransaction::MAX_STANDARD_VERSION)
        throw std::runtime_error("Invalid TX version requested");

    tx.nVersion = (int) newVersion;
}

static void MutateTxLocktime(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newLocktime = atoi64(cmdVal);
    if (newLocktime < 0LL || newLocktime > 0xffffffffLL)
        throw std::runtime_error("Invalid TX locktime requested");

    tx.nLockTime = (unsigned int) newLocktime;
}

static void MutateTxRBFOptIn(CMutableTransaction& tx, const std::string& strInIdx)
{
    // parse requested index
    int inIdx = atoi(strInIdx);
    if (inIdx < 0 || inIdx >= (int)tx.vin.size()) {
        throw std::runtime_error("Invalid TX input index '" + strInIdx + "'");
    }

    // set the nSequence to MAX_INT - 2 (= RBF opt in flag)
    int cnt = 0;
    for (CTxIn& txin : tx.vin) {
        if (strInIdx == "" || cnt == inIdx) {
            if (txin.nSequence > MAX_BIP125_RBF_SEQUENCE) {
                txin.nSequence = MAX_BIP125_RBF_SEQUENCE;
            }
        }
        ++cnt;
    }
}

static void MutateTxAddInput(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    // separate TXID:VOUT in string
    if (vStrInputParts.size()<2)
        throw std::runtime_error("TX input missing separator");

    // extract and validate TXID
    std::string strTxid = vStrInputParts[0];
    if ((strTxid.size() != 64) || !IsHex(strTxid))
        throw std::runtime_error("invalid TX input txid");
    uint256 txid(uint256S(strTxid));

    static const unsigned int minTxOutSz = 9;
    // Deprecated with RIP2 implementation
    //    static const unsigned int maxVout = MAX_BLOCK_WEIGHT / (WITNESS_SCALE_FACTOR * minTxOutSz);

    unsigned int maxVout = MAX_BLOCK_WEIGHT_RIP2 / (WITNESS_SCALE_FACTOR * minTxOutSz);

    // extract and validate vout
    std::string strVout = vStrInputParts[1];
    int vout = atoi(strVout);
    if ((vout < 0) || (vout > (int)maxVout))
        throw std::runtime_error("invalid TX input vout");

    // extract the optional sequence number
    uint32_t nSequenceIn=std::numeric_limits<unsigned int>::max();
    if (vStrInputParts.size() > 2)
        nSequenceIn = std::stoul(vStrInputParts[2]);

    // append to transaction input list
    CTxIn txin(txid, vout, CScript(), nSequenceIn);
    tx.vin.push_back(txin);
}

static void MutateTxAddOutAddr(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:ADDRESS
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    if (vStrInputParts.size() != 2)
        throw std::runtime_error("TX output missing or too many separators");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // extract and validate ADDRESS
    std::string strAddr = vStrInputParts[1];
    CTxDestination destination = DecodeDestination(strAddr);
    if (!IsValidDestination(destination)) {
        throw std::runtime_error("invalid TX output address");
    }
    CScript scriptPubKey = GetScriptForDestination(destination);

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutPubKey(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:PUBKEY[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    if (vStrInputParts.size() < 2 || vStrInputParts.size() > 3)
        throw std::runtime_error("TX output missing or too many separators");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // Extract and validate PUBKEY
    CPubKey pubkey(ParseHex(vStrInputParts[1]));
    if (!pubkey.IsFullyValid())
        throw std::runtime_error("invalid TX output pubkey");
    CScript scriptPubKey = GetScriptForRawPubKey(pubkey);

    // Extract and validate FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts[2];
        bSegWit = (flags.find("W") != std::string::npos);
        bScriptHash = (flags.find("S") != std::string::npos);
    }

    if (bSegWit) {
        if (!pubkey.IsCompressed()) {
            throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
        }
        // Call GetScriptForWitness() to build a P2WSH scriptPubKey
        scriptPubKey = GetScriptForWitness(scriptPubKey);
    }
    if (bScriptHash) {
        // Get the ID for the script, and then construct a P2SH destination for it.
        scriptPubKey = GetScriptForDestination(CScriptID(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutMultiSig(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:REQUIRED:NUMKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    // Check that there are enough parameters
    if (vStrInputParts.size()<3)
        throw std::runtime_error("Not enough multisig parameters");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // Extract REQUIRED
    uint32_t required = stoul(vStrInputParts[1]);

    // Extract NUMKEYS
    uint32_t numkeys = stoul(vStrInputParts[2]);

    // Validate there are the correct number of pubkeys
    if (vStrInputParts.size() < numkeys + 3)
        throw std::runtime_error("incorrect number of multisig pubkeys");

    if (required < 1 || required > 20 || numkeys < 1 || numkeys > 20 || numkeys < required)
        throw std::runtime_error("multisig parameter mismatch. Required " \
                            + std::to_string(required) + " of " + std::to_string(numkeys) + "signatures.");

    // extract and validate PUBKEYs
    std::vector<CPubKey> pubkeys;
    for(int pos = 1; pos <= int(numkeys); pos++) {
        CPubKey pubkey(ParseHex(vStrInputParts[pos + 2]));
        if (!pubkey.IsFullyValid())
            throw std::runtime_error("invalid TX output pubkey");
        pubkeys.push_back(pubkey);
    }

    // Extract FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == numkeys + 4) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find("W") != std::string::npos);
        bScriptHash = (flags.find("S") != std::string::npos);
    }
    else if (vStrInputParts.size() > numkeys + 4) {
        // Validate that there were no more parameters passed
        throw std::runtime_error("Too many parameters");
    }

    CScript scriptPubKey = GetScriptForMultisig(required, pubkeys);

    if (bSegWit) {
        for (CPubKey& pubkey : pubkeys) {
            if (!pubkey.IsCompressed()) {
                throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
            }
        }
        // Call GetScriptForWitness() to build a P2WSH scriptPubKey
        scriptPubKey = GetScriptForWitness(scriptPubKey);
    }
    if (bScriptHash) {
        // Get the ID for the script, and then construct a P2SH destination for it.
        scriptPubKey = GetScriptForDestination(CScriptID(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutData(CMutableTransaction& tx, const std::string& strInput)
{
    CAmount value = 0;

    // separate [VALUE:]DATA in string
    size_t pos = strInput.find(':');

    if (pos==0)
        throw std::runtime_error("TX output value not specified");

    if (pos != std::string::npos) {
        // Extract and validate VALUE
        value = ExtractAndValidateValue(strInput.substr(0, pos));
    }

    // extract and validate DATA
    std::string strData = strInput.substr(pos + 1, std::string::npos);

    if (!IsHex(strData))
        throw std::runtime_error("invalid TX output data");

    std::vector<unsigned char> data = ParseHex(strData);

    CTxOut txout(value, CScript() << OP_RETURN << data);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutScript(CMutableTransaction& tx, const std::string& strInput)
{
    // separate VALUE:SCRIPT[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));
    if (vStrInputParts.size() < 2)
        throw std::runtime_error("TX output missing separator");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // extract and validate script
    std::string strScript = vStrInputParts[1];
    CScript scriptPubKey = ParseScript(strScript);

    // Extract FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find("W") != std::string::npos);
        bScriptHash = (flags.find("S") != std::string::npos);
    }

    if (bSegWit) {
        scriptPubKey = GetScriptForWitness(scriptPubKey);
    }
    if (bScriptHash) {
        scriptPubKey = GetScriptForDestination(CScriptID(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxDelInput(CMutableTransaction& tx, const std::string& strInIdx)
{
    // parse requested deletion index
    int inIdx = atoi(strInIdx);
    if (inIdx < 0 || inIdx >= (int)tx.vin.size()) {
        std::string strErr = "Invalid TX input index '" + strInIdx + "'";
        throw std::runtime_error(strErr.c_str());
    }

    // delete input from transaction
    tx.vin.erase(tx.vin.begin() + inIdx);
}

static void MutateTxDelOutput(CMutableTransaction& tx, const std::string& strOutIdx)
{
    // parse requested deletion index
    int outIdx = atoi(strOutIdx);
    if (outIdx < 0 || outIdx >= (int)tx.vout.size()) {
        std::string strErr = "Invalid TX output index '" + strOutIdx + "'";
        throw std::runtime_error(strErr.c_str());
    }

    // delete output from transaction
    tx.vout.erase(tx.vout.begin() + outIdx);
}

class Secp256k1Init
{
    ECCVerifyHandle globalVerifyHandle;

public:
    Secp256k1Init() {
        ECC_Start();
    }
    ~Secp256k1Init() {
        ECC_Stop();
    }
};

static void MutateTx(CMutableTransaction& tx, const std::string& command,
                     const std::string& commandVal)
{
    std::unique_ptr<Secp256k1Init> ecc;

    if (command == "nversion")
        MutateTxVersion(tx, commandVal);
    else if (command == "locktime")
        MutateTxLocktime(tx, commandVal);
    else if (command == "replaceable") {
        MutateTxRBFOptIn(tx, commandVal);
    }

    else if (command == "delin")
        MutateTxDelInput(tx, commandVal);
    else if (command == "in")
        MutateTxAddInput(tx, commandVal);

    else if (command == "delout")
        MutateTxDelOutput(tx, commandVal);
    else if (command == "outaddr")
        MutateTxAddOutAddr(tx, commandVal);
    else if (command == "outpubkey") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutPubKey(tx, commandVal);
    } else if (command == "outmultisig") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutMultiSig(tx, commandVal);
    } else if (command == "outscript")
        MutateTxAddOutScript(tx, commandVal);
    else if (command == "outdata")
        MutateTxAddOutData(tx, commandVal);

    else
        throw std::runtime_error("unknown command");
}

static void OutputTxHash(const CTransaction& tx)
{
    std::string strHexHash = tx.GetHash().GetHex(); // the hex-encoded transaction hash (aka the transaction id)

    fprintf(stdout, "%s\n", strHexHash.c_str());
}

static void OutputTxHex(const CTransaction& tx)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    std::string strHex = HexStr(ssTx.begin(), ssTx.end());

    fprintf(stdout, "%s\n", strHex.c_str());
}

static void OutputTx(const CTransaction& tx)
{
    if (gArgs.GetBoolArg("-txid", false))
        OutputTxHash(tx);
    else
        OutputTxHex(tx);
}

static std::string readStdin()
{
    char buf[4096];
    std::string ret;

    while (!feof(stdin)) {
        size_t bread = fread(buf, 1, sizeof(buf), stdin);
        ret.append(buf, bread);
        if (bread < sizeof(buf))
            break;
    }

    if (ferror(stdin))
        throw std::runtime_error("error reading stdin");

    boost::algorithm::trim_right(ret);

    return ret;
}

static int CommandLineRawTx(int argc, char* argv[])
{
    std::string strPrint;
    int nRet = 0;
    try {
        // Skip switches; Permit common stdin convention "-"
        while (argc > 1 && IsSwitchChar(argv[1][0]) &&
               (argv[1][1] != 0)) {
            argc--;
            argv++;
        }

        CMutableTransaction tx;
        int startArg;

        if (!fCreateBlank) {
            // require at least one param
            if (argc < 2)
                throw std::runtime_error("too few parameters");

            // param: hex-encoded raven transaction
            std::string strHexTx(argv[1]);
            if (strHexTx == "-")                 // "-" implies standard input
                strHexTx = readStdin();

            if (!IsHex(strHexTx)) {
                throw std::runtime_error("invalid transaction encoding");
            }

            std::vector<unsigned char> txData(ParseHex(strHexTx));
            CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
            ssData >> tx;
            if (!ssData.empty()) {
                throw std::runtime_error("invalid transaction encoding");
            }

            startArg = 2;
        } else
            startArg = 1;

        for (int i = startArg; i < argc; i++) {
            std::string arg = argv[i];
            std::string key, value;
            size_t eqpos = arg.find('=');
            if (eqpos == std::string::npos)
                key = arg;
            else {
                key = arg.substr(0, eqpos);
                value = arg.substr(eqpos + 1);
            }

            MutateTx(tx, key, value);
        }

        OutputTx(tx);
    }

    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    }
    catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
        throw;
    }

    if (strPrint != "") {
        fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
    }
    return nRet;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();

    try {
        int ret = AppInitRawTx(argc, argv);
        if (ret != CONTINUE_EXECUTION)
            return ret;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRawTx()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRawTx()");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    try {
        ret = CommandLineRawTx(argc, argv);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRawTx()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
    }
    return ret;
}
