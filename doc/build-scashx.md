# SCASHX BUILD NOTES

## ScashX: Bitcoin Fork for Home Mining

ScashX (Satoshi Cash-X) is a modified fork of Bitcoin Core v27.0 which adds a new chain option to make cryptocurrency mining feasible again on standard home computers.

## Technical Foundation

Built upon Bitcoin Core v27.0, ScashX shares its core features and rules. For specific technical changes, please refer to the ScashX Protocol spec [ScashX Protocol spec](https://github.com/scashx/scashx/blob/scashx_master/doc/scashx-protocol-spec.md).

## Availability and Building

Binary releases: https://github.com/scashx/scashx/releases

Building ScashX follows the same instructions as building Bitcoin. The ScashX network shares the same features and rules as Bitcoin mainnet, as specified in Bitcoin Core v27.0.

### Linux

The Linux version of the node (`scashxd`) is supported and also runs in Ubuntu on [Windows Subsystem for Linux (WSL)](https://learn.microsoft.com/en-us/windows/wsl/about).

### Windows

Windows users will build from source by running Ubuntu WSL and following the Linux instructions.

### Other Platforms

A GUI app `scashx-qt` will be supported in a future release, with Windows binaries made available (cross-compiled on Linux). MacOS is not yet supported.

# Building

The instructions in this document are sufficient for a rapid install of ScashX, taking approximately ten minutes, depending on processor performance. For more specific instructions on building, see [`build-unix.md`](build-unix.md) in this directory.

Also see the latest [ScashX release notes](release-notes/scashx/).

## Getting started 

Update your system and install the following tools required to build software.

```bash
sudo apt update
sudo apt upgrade
sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git cmake bison
```

## WSL for Windows

Ignore this step if building on native Linux. The following only applies when building in WSL for Windows.

Open the WSL configuration file:
```bash
sudo nano /etc/wsl.conf
```
Add the following lines to the file:
```
[interop]
appendWindowsPath=false
```
Exit WSL and then restart WSL ('wsl --shutdown' from Windows Command Prompt).

## Downloading the code

Download the latest version of ScashX.

```bash
git clone https://github.com/scashx/scashx.git
cd scashx
```

## Building for Linux

ScashX requires building with the depends system.

When calling `make` use `-j N` for N parallel jobs, as many as compatible with your processor. Type 'nproc' to see the available processors on your computer.

### Node software

To build the node software `scashxd`:

```bash
./autogen.sh
make -C depends NO_QT=1
./configure --without-gui --prefix=$PWD/depends/x86_64-pc-linux-gnu --program-transform-name='s/bitcoin/scashx/g'
make
make install
```
### Executables

The compiled executables will be found in `depends/x86_64-pc-linux-gnu/bin/` and can be copied to a folder on your path, typically `/usr/local/bin/` or `$HOME/.local/bin/`.

## Config file

The ScashX configuration file is the same as bitcoin.conf.

By default, Scash looks for a configuration file here:
`$HOME/.scashx/scashx.conf`

The following is a sample `scashx.conf`:

```
# Global Settings
rpcuser=user
rpcpassword=password
chain=scashx
daemon=0            # Disable running node as daemon
debug=0             # Disable debug logging
listen=1            # Enable listen for incoming connections
txindex=1           # Enable transaction indexing
randomxfastmode=1   # Enable fast mode for RandomX
fallbackfee=0.01    # Set fallback fee

# Mainnet Configuration
[scashx]
adddnsseed=seed.scashx.io
adddnsseed=seed2.scashx.io

# Testnet Configuration
[scashxtestnet]
addnode=45.76.143.162

# Regtest Configuration
[scashxregtest]
addnode=45.76.143.162

```

### Connecting to the network

To help find other nodes on the network, a [DNS seed](https://bitcoin.stackexchange.com/questions/14371/what-is-a-dns-seed-node-vs-a-seed-node) has been specified. Users are can also ask the community for a list of [reliable DNS seeds](https://github.com/bitcoin/bitcoin/blob/master/doc/dnsseed-policy.md) to use, as well as the IP addresses of stable nodes on the network which can be used with the `-addnode` and `-seednode` RPC calls.

If you intend to use the same configuration file with multiple networks, the config sections are named as follows:
```
[btc]
[btctestnet3]
[btcsignet]
[btcregtest]
[scashx]
[scashxregtest]
[scashxtestnet]
```

## Running a node

To run the ScashX node:
```bash
scashxd
```

To send commands to the ScashX node:
```
scashx-cli [COMMAND] [PARAMETERS]
```

```
Also note that in WSL for Windows, by default only half of the memory is available to WSL. You can [configure the memory limit](https://learn.microsoft.com/en-us/windows/wsl/wsl-config#main-wsl-settings) by creating `.wslconfig` file in your user folder.
```
[wsl2]
memory=16GB
```

## Connecting to different chains

When running executables with the name `bitcoin...` if no chain is configured, the default chain will be Bitcoin mainnet.

When running executables with the name `scashx...` if no chain is configured, the default chain will be ScashX mainnet.

Option `-chain=` accepts the following values: `scashx` `scashxtestnet` `scashxregtest` and for Bitcoin networks: `main` `test` `signet` `regtest`

## Mining ScashX

There are a few ways to mine ScashX.

### Main network and Testnet chain

Mining takes place inside [cpuminer-scashx](https://github.com/scashx/cpuminer-scashx) which is dedicated mining software that connects to the ScashX node and retrieves mining jobs via RPC `getblocktemplate`. The 'randomxfastmode' configuration option is not required for the ScashX node, since mining occurs inside `cpuminer-scashx` which always runs in fast mode.

Build instructions for [cpuminer-scashx](https://github.com/scashx/cpuminer-scashx/blob/master/README.md).

### Testnet and Regtest chain

Mining takes place inside the ScashX node, using the RPC `generatetoaddress` which is single-threaded. For example:
```bash
scashx-cli createwallet myfirstwallet
scashx-cli getnewaddress
scashx-cli generatetoaddress 1 newminingaddress 10000
```

To speed up mining in the ScashX node, at the expense of using more memory (at least 2GB more), enable the option `randomxfastmode` by adding to the `scashx.conf` configuration file:

```
randomxfastmode=1
```

### Mining Pools

Third-party software exists for mining at pools.

Getting Help
---------------------

Please file a Github issue if build problems are not resolved after reviewing the available ScashX and Bitcoin documentation.
