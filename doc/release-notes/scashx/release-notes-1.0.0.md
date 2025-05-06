1.0.0 Release Notes
===================

ScashX can be built from source and pre-compiled binaries are available.

ScashX versioning is as follows:
```
ScashX version v1.x.x-novaelis-core-26.0.0 
              |          |            |
            SCASH     CODENAME    BITCOIN CORE
```

Please report bugs using the issue tracker at GitHub:

  <https://github.com/scashx/scashx/issues>

How to Upgrade
==============

If you are running an older version of ScashX, shut it down:
```
./scashx-cli stop
```
Wait until it has completely
shut down (which might take a few minutes in some cases). Then just copy over
`scashxd` (on Linux). and startup the new node:
```
./scashxd
```

Compatibility
==============

ScashX is built as a new chain type on top of the Bitcoin Core software. ScashX
can connect to the Bitcoin network and operate as a Bitcoin client fully compatible with the current network consensus rules. However, it is not recommended to use ScashX
as a Bitcoin client, and instead Bitcoin Core should be used.

ScashX is supported and tested on operating systems using the Linux kernel.
ScashX should also work on most other Unix-like systems but is not as frequently tested
on them.  It is not recommended to use ScashX on unsupported systems.

ScashX is based on Bitcoin Core 27.0. See a list of changes in the [Bitcoin Release Notes](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-27.0.md).

Notable changes
===============

Proof of work
-------------
- The SHA256 proof of work has been replaced with RandomX.  See the the [Scash Protocol spec](https://github.com/scashx/scashx/blob/scashx_master/doc/scashx-protocol-spec.md).

ASERT Difficulty Adjustment Algorithm
-------------------------------------
- Difficulty adjustment algorithm (DAA) is activated. The old DAA, inherited from Bitcoin (BTC), has been replaced with a new DAA called [ASERT (aserti3-2d)](https://reference.cash/protocol/forks/2020-11-15-asert) used by Bitcoin Cash (BCH). The ASERT DAA is more responsive to fluctuating hashrate and adjusts every block instead of every 2016 blocks.

Replace-by-fee
-------------- 
- Disabled when running the ScashX network

Datacarrier
------------
- Disabled when running the ScashX network

Ordinals
--------
- Transactions containing ordinals inscriptions are treated as non-standard when running the ScashX network.

New options
-----------

- New node option `-suspiciousreorgdepth` has been added to help protect against deep reorgs. Upon detection of a suspicious reorg, the node will shut down for safety and a human operator can then decide what to do e.g. allow the reorg, invalidate blocks, upgrade software, etc. By default, a reorg depth of 100 is treated as suspicious. This is the same as coinbase maturity and protects newly spendable coinbase rewards from being invalidated.
  
- New node option `-adddnsseed` has been added so users can add DNS seeds to query for addresses of nodes via DNS lookup. This option can be specified multiple times to connect to multiple DNS seeds. Hardcoded DNS seeds have been removed.
  
- New chain options `-scashx`, `-scashxtestnet`, `-scashxregtest`

- New proof of work related options `-randomxfastmode` and `-randomxvmcachesize`.
  See the [ScashX Protocol spec](https://github.com/scashx/scashx/blob/scashx_master/doc/scashx-protocol-spec.m).

Updated RPCs
------------

- `getblock` RPC returns new fields `rx_cm`, `rx_hash`, `rx_epoch`

- `getblocktemplate` RPC returns new field `rx_epoch_duration`

Miscellaneous
-------------
- RandomX fast mode now activates after initial block download completes
- Fix RPC help/error messages
- Known issue
  - Building with configure option --disable-wallet currently fails.



