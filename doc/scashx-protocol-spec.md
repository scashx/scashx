# ScashX Protocol

#### Authors

Original Authors: Simon Liu (Scash project)

Original Created: 2024-02-16

Original License: BSD-2-Clause


Authors: The Satoshi Cash-X developers (Forked from Scash)

Modifications Started: 2025-01-05

License (for modifications): BSD-2-Clause

#### Copyright

Copyright (c) 2024, Simon Liu

Copyright (c) 2025, The Satoshi Cash-X developers

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Abstract

ScashX (Satoshi Cash-X) is an experimental digital currency based on the Bitcoin protocol.

ScashX is designed for mining on home computers.

ScashX has been implemented as a new chain option on top of v27 of the Bitcoin Core open source software.

This document details the changes required to add support for ScashX to Bitcoin related software.


#### Table of contents

1. [Proof of work](#1-proof-of-work-algorithm)
1. [Block hashing](#2-block-hashing)
1. [Block validation](#3-block-validation)
1. [Algorithm performance](#4-algorithm-performance)
1. [Difficulty adjustment](#5-difficulty-adjustment)
1. [Transactions](#6-transactions)
1. [Chain parameters](#7-chain-parameters)
1. [JSON-RPC fields](#8-json-rpc-fields)


## 1. Proof of work algorithm

### 1.1 RandomX

ScashX replaces Bitcoin's SHA256 proof of work algorithm with [RandomX](https://github.com/tevador/RandomX), a proof of work algorithm designed to close the gap between general-purpose CPUs and specialized hardware. ScashX uses v1.2.1 of RandomX.

### 1.2 Parameters

ScashX customizes the standard configuration parameters for RandomX, with the following change:

|Parameter|Description|Value|
|---------|-----|-------|
|`RANDOMX_ARGON_SALT`|Argon2 salt|`"RandomX-ScashX\x01"`|

### 1.3 Epoch

The key `K` input to the RandomX algorithm changes over time.

ScashX divides time into epochs.

The epoch `E` is calculated as `unix_timestamp / epoch_duration` using integer math.

ScashX defines the epoch duration as follows:

|Chain|Seconds|
|---|---|
|ScashX| `7 * 24 * 60 * 60`|
|ScashX Testnet| `7 * 24 * 60 * 60`|
|ScashX Regtest| `1 * 24 * 60 * 60`|

### 1.4 Key Derivation

To derive key `K` to be used as input to the RandomX algorithm, perform the following steps:

1. Compute epoch `E` as an integer value.
2. Generate a seed string `S` by substituting epoch `E` into string `"ScashX/RandomX/Epoch/%d"` (`E` replaces `%d`).
3. Key `K` is the digest of `SHA256(SHA256(S))`.

Note: Using an epoch-based predefined sequence of keys instead of deriving keys from historical blockchain data enables light clients to fully validate the proof of work in a block header without requiring any contextual data from the blockchain.


## 2 Block hashing

### 2.1 Block header

The ScashX block header extends the Bitcoin block header with a new field `hashRandomX` to store the result of executing the RandomX algorithm.

| Field | Size (Bytes) |
|---|---|
| Version | 4 |
| hashPrevBlock | 32 |
| hashMerkleRoot | 32 |
| Time | 4 |
| Bits | 4 |
| Nonce | 4 |
| hashRandomX | 32 |

### 2.2 Block hash

The block hash algorithm remains the same as Bitcoin, double SHA256 over the entire block header: `SHA256(SHA256(block_header)`.

The block hash is not used to validate proof of work.

### 2.3 RandomX hash

RandomX hashing requires two input values:

* Key `K` with a size of 32 bytes
* Block header `H` with a size of 112 bytes
  * set the `hashRandomX` field to null (all zero)

and outputs a 256-bit result `R`.

The hashing process consists of the following steps:

1. Compute the epoch `E` from the block header field `Time`.
2. Compute the seed string `S` from epoch `E`.
3. Derive key value `K` from the seed string `S`.
4. Result `R` is calculated by executing the RandomX algorithm with key `K` and block header `H`.

Result `R` is stored in the block header field `hashRandomX`.

### 2.4 RandomX commitment (Corrected Section Number)

Calculating the RandomX commitment value requires two input values:

* Hash value `hashRandomX` with a size of 32 bytes, for a block header `H`
* Block header `H` with a size of 112 bytes
  * set the `hashRandomX` field to null (all zero)

and outputs a 256-bit result `CM`.

The commitment value `CM` is transient and is not stored in the block header.


## 3 Block validation

A block header has valid proof of work when both:

1. The RandomX commitment value `CM` for the block header meets the current target.
2. The RandomX hash value `hashRandomX` in the block header is verified.

### 3.1 Commitment value meets target

In Bitcoin, the block hash must be lower than or equal to the current target `T`, a large 256-bit number, for the block to be accepted by the network.

In ScashX, instead of using the block hash, the RandomX commitment value `CM` must be lower than or equal to the current target `T`:
- `CM <= T`, meets the target
- `CM > T`, does not meet the target

### 3.2 Verifying RandomX hash value

To verify the block header field `hashRandomX`, perform the RandomX hashing algorithm over the block header, as described earlier.

Compare the result `R` against the value in the block header:
- `R == hashRandomX`, block header is valid
- `R != hashRandomX`, block header is invalid

### 3.3 Light clients

Light clients may choose to only verify the commitment `CM` and skip the more computationally expensive verification of `hashRandomX` when processing block data from trusted sources.

Trusted sources such as full nodes must perform full verification of both values before accepting blocks, given it is possible to construct a block header with a fake `hashRandomX` which generates a valid commitment `CM`.


## 4 Algorithm performance

The ScashX node software provides options to adjust RandomX performance.

| Option | Purpose | Default |
|---|---|:---:|
| randomxfastmode | Enable fast mode | false |
| randomxvmcachesize | Number of epochs/VMs to cache | 2 |

### 4.1 Fast mode

The RandomX library provides a fast mode but this greatly increases the amount of memory used by the RandomX algorithm:
- Fast mode - requires 2080 MiB of shared memory, suitable for mining.
- Light mode - requires 256 MiB of shared memory, but runs slower.

### 4.2 Caching

Every key `K` requires its own uniquely initialized RandomX virtual machine to execute the RandomX algorithm.

Since block timestamps `Time` are not guaranteed to increase monotonically, caching can eliminate the expense of recreating a RandomX virtual machine when epochs are out of order.


## 5 Difficulty adjustment

The implementation is based on the Bitcoin Cash ASERT Difficulty Adjustment Algorithm (aserti3-2d) specification [1], potentially incorporating minor modifications like `uint512` usage for overflow handling and additional test vectors (see also sip0010.md).

### 5.1 Algorithm

Block production in ScashX, with an ideal target time of 10 minutes, can be impacted by hash power fluctuation. The ASERT DAA adjusts the network difficulty every block, helping to restore block production to the desired target rate more quickly than the legacy DAA, which only adjusts network difficulty every 2016 blocks. This improves consistency in block times and transaction confirmation reliability. More rationale is provided by the original ASERT DAA authors [2].

The proposed anchor block for the ASERT calculation is ScashX block height 50 and the activation is at 70.

#### Implementation

The reference implementation for ASERT DAA is found in Bitcoin Cash Node:

- `pow.cpp`: https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/blob/4ee1083307d2aaac92dd7c409cc9d6f2eb52be78/src/pow.cpp

- `pow_tests.cpp`: https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/blob/0a5fa6246387c3a9498898ee5257ee6950c1b635/src/test/pow_tests.cpp

#### References

[1] ASERT Specification (Bitcoin Cash): https://reference.cash/protocol/forks/2020-11-15-asert

[2] ASERT Motivation/Rationale (J. Toomim): https://read.cash/@jtoomim/bch-upgrade-proposal-use-asert-as-the-new-daa-1d875696


## 6 Transactions

The ScashX node software makes a number of changes to the default behaviour of the Bitcoin node software. These changes impact transaction propagation across the ScashX network.

### 6.1 Replace-by-fee

The ScashX node software disables replace-by-fee (RBF).

The mempool returns to first-seen-rule behaviour and rejects conflicting transactions.

- Option `-mempoolfullrbf` disabled
- Option `-walletrbf` disabled
- RPC argument `replaceable` disabled

### 6.2 Data carrier

The ScashX node software disables the `-datacarrier` option.

Non-coinbase transactions containing an `OP_RETURN` transaction output remain consensus valid and can be mined into blocks, but they will not be relayed by the node.

ScashX discourages the use of the network for storing data not required to validate a transaction.

### 6.3 Ordinals Inscriptions

The ScashX node software considers a transaction as non-standard when the following dead code patterns are detected in Tapscript:
- `OP_FALSE OP_IF`
- `OP_NOTIF OP_TRUE`

Non-standard transactions remain consensus valid and can be mined into blocks, but they will not be relayed by the node.

ScashX discourages the use of the network for storing data not required to validate a transaction.

## 7 Chain parameters

The ScashX chain modifies the default Bitcoin chain parameters.

### 7.1 Network handshake

ScashX adds `0x02` to each of the network magic bytes used by Bitcoin, to define the following defaults:

| Chain | Magic bytes | Notes (Bitcoin + 0x02) |
|---|---|---|
| ScashX | `0xfb 0xc0 0xb6 0xdb` | `0xf9beb4d9` + `0x02020202` |
| ScashX Testnet | `0x0d 0x13 0x0b 0x09` | `0x0b110907` + `0x02020202` |
| ScashX Regtest | `0xfc 0xc1 0xb7 0xdc` | `0xfabfb5da` + `0x02020202` |

### 7.2 Network ports

ScashX adds `20` to the default Bitcoin port numbers, to define the following defaults:

| Chain | RPC | Network | Tor | Notes (Bitcoin + 20) |
|---|---|---|---|---|
| ScashX | `8352` | `8353` | `8354` | BTC: 8332, 8333, 9050/9150 |
| ScashX Testnet | `18352` | `18353` | `18354` | BTC: 18332, 18333 |
| ScashX Regtest | `18463` | `18464` | `18465` | BTC: 18443, 18444 |
*(Note: Tor port assignments may need review)*

### 7.3 Bech32 prefix

ScashX updates the human readable prefix for Bech32 addresses as follows:

| Chain | Prefix |
|---|:---:|
| ScashX | scashx |
| ScashX Testnet | tscashx |
| ScashX Regtest | rscashx |

## 8 JSON-RPC fields

The ScashX node software adds RandomX data fields to the following API endpoints.

| API  | Key | Value | Description |
|---|---|---|---|
| `getblock` | `"rx_cm"` | Hex string | RandomX commitment value |
|  | `"rx_hash"` | Hex string | RandomX hash value |
|  | `"rx_epoch"` | Integer | Epoch |
| `getblocktemplate` | `"rx_epoch_duration"` | Integer | Epoch duration in seconds |

[1] ASERT Specification (Bitcoin Cash): https://reference.cash/protocol/forks/2020-11-15-asert
[2] ASERT Motivation/Rationale (J. Toomim): https://read.cash/@jtoomim/bch-upgrade-proposal-use-asert-as-the-new-daa-1d875696
