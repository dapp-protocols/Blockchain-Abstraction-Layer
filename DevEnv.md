# How to Set Up a BAL Contract Development Environment

In this document, we will explore how to set up a development environment for building BAL contracts on the developer's host OS. This is in contrast to other documentation covering how to set up a containerized development environment which uses Docker containers to ship the toolchains and nodes used in the BAL contract development process: these instructions cover how to build the necessary componentry from scratch and install them into your local operating system.

This tutorial is written for developers experienced with Linux operating systems, who are accustomed to compiling software from source and have at least basic command line proficiency. It makes minimal assumptions about the reader's environment and aims to provide generally applicable instructions which can be readily interpreted for any Unix-like environment.

### EOSIO Development Environment
The EOSIO contract development environment is compiled in three steps. The first step is to build LLVM with the necessary flags for EOSIO. The second step is to build the EOSIO node, which will be used to launch local test blockchain networks to deploy and test the prototype contract on. The third step is to build and install the EOSIO Contract Development Toolkit (CDT), which is the toolchain and standard libraries for EOSIO contract development. Once the CDT is installed, we can compile a contract and load it into our testnet for testing.

##### LLVM/Clang
LLVM/Clang can be built according to the official [instructions](https://clang.llvm.org/get_started.html). Note that, as EOSIO does not support the latest versions of LLVM, it is necessary to use an older version. At the time of this writing, version 11.1 is the latest supported version. This can be selected in the initial clone command by passing `--branch llvmorg-11.1.0`.

When building, it is necessary to add the following flags when running CMake to build it for EOSIO:

```cmake
-DLLVM_ENABLE_RTTI=1 -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
'-DLLVM_ENABLE_PROJECTS=clang;compiler-rt' -DCMAKE_INSTALL_PREFIX=/opt/clang-eosio
```

I also like to add the `-DCMAKE_INSTALL_PREFIX=/opt/clang-eosio` flag so that this copy of LLVM is set aside as a special build for EOSIO and does not interfere with my main (packaged) LLVM.

> **Troubleshooting:**
> When building LLVM 11.1.0 in an updated environment, some build errors may occur due to the environment being more recent than the target LLVM. These compatibility issues have been fixed in updated versions of LLVM, and can be backported to LLVM 11.1.0 by cherry picking commits.
>  - `llvm-project/compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.cpp:133:10: fatal error: linux/cyclades.h: No such file or directory`
>    - Run `git cherry-pick 884040db086936107ec81656aa5b4c607235fb9a`
>    - Ref https://github.com/llvm/llvm-project/commit/884040db086936107ec81656aa5b4c607235fb9a
>  - `llvm-project/llvm/utils/benchmark/src/benchmark_register.h:17:30: error: ‘numeric_limits’ is not a member of ‘std’`
>    - Run `git cherry-pick b498303066a63a203d24f739b2d2e0e56dca70d1`
>    - Ref https://github.com/llvm/llvm-project/commit/b498303066a63a203d24f739b2d2e0e56dca70d1

##### Building EOSIO Node
Clone the EOSIO node source code repository from `https://github.com/eosio/eos`, create a `build` directory within the repository, and `cd` into it. Run CMake to configure the build. Here's an example of some commands to do this:

```sh
$ git clone https://github.com/eosio/eos --recursive
$ mkdir eos/build
$ cd eos/build
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/opt/clang-eosio/lib/cmake/llvm \
  -DCMAKE_INSTALL_PREFIX=/opt/eos -DCMAKE_INSTALL_SYSCONFDIR=/opt/eos/etc \
  -DCMAKE_INSTALL_LOCALSTATEDIR=/opt/eos/var ..
```

Note the `--recursive` flag on the clone. This is important, as it causes git to clone all of the submodules during the initial clone. If you forget it, you'll need to get them all after the fact by running `git submodule update --init --recursive`.

Also note the many flags to CMake. The `-G Ninja` causes CMake to write Ninja build files instead of Makefiles, as I prefer Ninja. Readers who prefer to use make should omit this flag. The build type can be adjusted to the reader's preference. The `LLVM_DIR` flag indicates to EOSIO where our custom built clang is. The `CMAKE_INSTALL_*` flags are used to cause EOSIO to install to `/opt/eos` rather than `/usr/local`, which I find preferable as it keeps my main OS directories clear of files not managed by my package manager.

> **Troubleshooting:**
> - In environments which have only dynamic Boost libraries, CMake will fail to find Boost libraries saying "no suitable build variant has been found". Work around this issue by adding the CMake argument `-DBoost_USE_STATIC_LIBS=OFF`
> - In some environments, building fails with errors circa a `hash<>` template referenced from FC. Work around this issue by adding a `#include <boost/container_hash/hash.hpp>` to `libraries/fc/include/fc/crypto/sha256.hpp`
> - In some environments, building fails due to missing `std::list` in FC. Work around this issue by adding a `#include <list>` to `libraries/fc/include/fc/io/raw.hpp`
> - `eos/libraries/eos-vm/include/eosio/vm/execution_context.hpp:273:61: error: no matching function for call to ‘max(int, long int)’`
>   - Manually apply [this commit](https://github.com/EOSIO/eos-vm/pull/228/commits/6fd38668d6fa1a909fb1322b676ba43ff56f14e2) to eos-vm in `libraries/eos-vm` directory
>   - Ref https://github.com/EOSIO/eos-vm/issues/227

After configuring, build and install the node, for example with `ninja && ninja install`

##### Launching an EOSIO Testnet
Now that the EOSIO node is installed, we launch a private testnet so we can produce blocks and test our contract locally. The command is as follows:

```sh
$ export PATH="$PATH:/opt/eos/bin"
$ nodeos --plugin eosio::http_plugin --plugin eosio::chain_api_plugin \
  --http-server-address 0.0.0.0:8888 --p2p-listen-endpoint 0.0.0.0:9889 \
  --contracts-console -e -p eosio
```

The meanings of the arguments are:
- The plugins, although optional, are useful for getting chain state information out of the node
- `--http-server-address <host>` directs nodeos to listen for RPC calls on port 8888
- `--p2p-listen-endpoint <host>` directs nodeos to listen for peer nodes on port 9889
- `--contracts-console` directs nodeos to print contract messages to the console
- `-e` directs nodeos to produce blocks even if the head block is quite old (this is safe and normal on a test net, but not on a live net)
- `-p eosio` means to produce blocks with the `eosio` account

This should result in a node that produces blocks. Press `Ctrl-C` to cleanly stop the node.

##### Building EOSIO CDT
Similarly to the node, clone the repo, make a build directory, and configure. Note that, pending inclusion of some fixes upstream, it is necessary to use the DApp Protocols fork to get full support.

```sh
$ git clone https://github.com/dapp-protocols/eosio-cdt --recursive
$ mkdir eosio.cdt/build
$ cd eosio.cdt/build
$ export CMAKE_PREFIX_PATH="/opt/eos:$CMAKE_PREFIX_PATH"
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/eosio-cdt ..
```

The options here are similar to the options in the node above, and follow the same rules; however, it is necessary to add the CMAKE_PREFIX_PATH as an environment variable as the CDT's cmake configuration does not properly propagate this value if passed to CMake directly.

> **Troubleshooting:** If it was necessary to specify `-DBoost_USE_STATIC_LIBS=OFF` when building EOSIO above, it will be necessary here as well. Moreover, when building fails even with that flag set, edit `build/tests/integration/CMakeCache.txt` and set `Boost_USE_STATIC_LIBS=ON` to `OFF` as well, then try building again.

Now, build and install the CDT, i.e. `ninja && ninja install`. Note that this build takes quite a while and is usually working even when it doesn't appear as such.

Note: Like with the LLVM build above, the CDT builds a custom LLVM for building the contracts, and this LLVM also fails to build for me with errors around `std::numeric_limits`. These can also be fixed by adding `#include <limits>` in the beginning of the affected files.

##### Building a Contract for EOSIO
Now that the EOSIO build environment is established, we can use it to build a contract. Let's build the BAL's example supply chain contract.

First, check out the BAL repo, make a `build` directory in the `Example` subdirectory, then use CMake to configure the build, and finally, build the contract.

```sh
$ git clone https://github.com/dapp-protocols/blockchain-abstraction-layer bal
$ mkdir bal/Example/build
$ cd bal/Example/build
$ cmake -G Ninja -DCDT_PATH=/opt/eosio-cdt ..
$ ninja
$ ls BAL/EOSIO/supplychain
```

The contract is built into the `BAL/EOSIO` directory. Unfortunately, the abigen doesn't handle our tagged IDs quite as nicely as we would like, so it is ideal to edit the auto generated ABI in `BAL/EOSIO/supplychain/supplychain.abi` to change the types of the tagged IDs from their struct types to plain `uint64`s. For example, change `CargoId`'s definition from this:

```
         {
             "new_type_name": "CargoId",
             "type": "ID_NameTag_4732942359761780736"
         }
```
... into this:
```
         {
             "new_type_name": "CargoId",
             "type": "uint64"
         }
```

This should be done for `CargoId`, `InventoryId`, `ManifestId`, and `WarehouseId` (but not `TransactionId`!). After this, it is ideal to remove the `ID_NameTag_#####` entries from the `structs` section entirely. While these edits to the ABI are not necessary, if they are omitted, IDs in the contract must be written as, for example, `{"value": 7}` rather than simply `7`. 

##### Deploying and Running an EOSIO Contract
To deploy and run a contract on EOSIO, first ensure that `nodeos` is up and producing blocks as described [above](#launching-an-eosio-testnet). With the testnet operational, run the following commands to prepare the contract account, load the contract, and test it:

```sh
$ cleos wallet create --file wallet.key # Create wallet
$ cleos wallet open # Open the wallet
$ cleos wallet unlock < wallet.key # Unlock the wallet with key in file
$ # Import the default testnet private key into wallet
$ cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
$ cleos wallet create_key # Create a new key pair in the wallet
$ # Create an account for the contract, controlled by the key we just created
$ cleos create account eosio supplychain <public key from last command>
$ # Deploy contract to supplychain account
$ cleos set contract supplychain <path to contract build>/BAL/EOSIO/supplychain
$ cleos wallet create_key # Create a key for a new account
$ # Create an accout to test contract, controlled by the key we just created
$ cleos create account eosio tester <public key from last command>
$ # Invoke contract to create a warehouse for tester account
$ cleos push action supplychain add.wrhs \
    '{"manager": "tester", "description": "Tester Warehouse"}' -p tester@active
$ cleos get table supplychain global warehouses # Read warehouses table
```

The output of the last command shows us the contents of the `supplychain` contract's `warehouses` table in the `global` scope:

```json
{
  "rows": [{
      "id": 0,
      "manager": "tester",
      "description": "Tester Warehouse"
    }
  ],
  "more": false,
  "next_key": "",
  "next_key_bytes": ""
}
```

Here we see that our new warehouse _Tester Warehouse_ is entered as being managed by `tester` and has the ID `0`. Now tester can create inventory in his warehouse:

```sh
$ cleos push action supplychain add.invntry '{"warehouseId": 0, "manager": "tester", "description": "Reticulated Frobulator", "quantity": 1000}' -p tester@active
$ cleos get table supplychain 0 inventory
```

The last command shows us the contents of the `inventory` table of the `supplychain` contract in the scope of warehouse ID `0`:

```json
{
  "rows": [{
      "id": 0,
      "description": "Reticulated Frobulator",
      "origin": "769d937144637f9e5c174c08d469143193f11df3a8b90f72864df990eeee449c",
      "movement": [],
      "quantity": 1000
    }
  ],
  "more": false,
  "next_key": "",
  "next_key_bytes": ""
}
```

We see that our contract is working properly. Now let's try to make a new account and use it to remove inventory from `tester`'s warehouse:

```sh
$ cleos wallet create_key
$ cleos create account eosio tester2 <key from above>
$ cleos push action supplychain adj.invntry '{"warehouseId": 0, "manager": "tester", "inventoryId": 0, "newDescription": null, "quantityAdjustment": ["int32", -10], "documentation": "Theft!!!"}' -p tester2@active
```

We see that this action fails as desired; however, if we do it as `tester`, it works perfectly:

```sh
$ cleos push action supplychain adj.invntry '{"warehouseId": 0, "manager": "tester", "inventoryId": 0, "newDescription": null, "quantityAdjustment": ["int32", -10], "documentation": "Sales"}' -p tester@active
$ cleos get table supplychain 0 inventory
```

We see the inventory is adjusted as intended:

```json
{
  "rows": [{
      "id": 0,
      "description": "Reticulated Frobulator",
      "origin": "769d937144637f9e5c174c08d469143193f11df3a8b90f72864df990eeee449c",
      "movement": [],
      "quantity": 990
    }
  ],
  "more": false,
  "next_key": "",
  "next_key_bytes": ""
}
```

We have now deployed and tested our contract by running actions on it and reading the tables to see that the actions are processed as intended.

### Peerplays Development Environment
The Peerplays development environment is built in two steps: first, the Follow My Vote fork of the official Peerplays node is compiled and installed. The Follow My Vote fork is patched to provide additional features necessary for third party contracts. This node is used to launch a local testnet. Second, Follow My Vote's custom Peerplays node is built. This node then connects to the testnet and is used to deploy BAL contracts and track their state.

##### Building the Patched Peerplays Node
Clone and build the node from Follow My Vote's fork using the following commands:

```sh
$ # Clone repo
$ git clone --recursive https://github.com/dapp-protocols/peerplays
$ # Make a build directory and move into it
$ mkdir peerplays/build
$ cd peerplays/build
$ # Configure the project
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/peerplays -DGRAPHENE_BUILD_DYNAMIC_LIBRARIES=ON ..
$ ninja # Build!
$ ninja install # Install
```

The cmake flag `-G Ninja` means to use Ninja instead of Make. This is optional. The `CMAKE_BUILD_TYPE` and `CMAKE_INSTALL_PREFIX` can be adjusted to the reader's preference. The final flag, `GRAPHENE_BUILD_DYNAMIC_LIBRARIES`, is required and confiures the node to build dynamic rather than static libraries.

If the build fails with cryptic linker errors saying to compile with `fPIC`, it is possible that the first cmake run did not have the `GRAPHENE_BUILD_DYNAMIC_LIBRARIES` flag specified correctly. Try deleting `CMakeCache.txt` and running cmake again being sure to set this flag as specified above.

##### Launching a Peerplays Testnet
To launch a Peerplays testnet, create a directory called `testnet-data` to store the testnet node's state. Now, create a Genesis file in that directory by running `/opt/peerplays/bin/witness_node --create-genesis-json testnet-data/genesis.json`. The Genesis file contains the initial state of the blockchain before the first block, including the initial distribution of tokens, the initially extant accounts, operation fees, witnesses, committee members, configurable settings, etc. For our purposes, the most relevant sections are `initial_accounts`  and `initial_witness_candidates`. These can be adjusted before launching the testnet to define accounts which can be used during testing. In the default testing Genesis, the `init#` witness accounts are created, and an additional `nathan` account. The reader may adjust these accounts and their keys to their preference, but the instructions below will assume these default values.

> Historical note: The observant reader may wonder if there is a relationship between the default `nathan` account and the author of the BAL, Nathaniel. The default testing account is indeed named after me. I was one of the original developers of the Graphene Blockchain Framework, the foundation of blockchains such as BitShares, Peerplays, Steem, EOSIO, and others. I was also the one who, circa 2015, created much of the testing frameworks used by Graphene blockchains to this day, including the testnet features such as an external Genesis file. At the time, I named the default testing account after myself (as I was the one usually using it), and that naming has persisted to the present day.

After creating the testnet Genesis state, add a `config.ini` next to it with the following contents:

```ini
# Use the prepared Genesis file
genesis-json = <full path to genesis.json>
# Testnet uses no seed nodes
seed-nodes = []
# Enable block production on empty/stale chain
enable-stale-production = true
# Produce blocks using init witnesses
witness-ids = ["1.6.0", "1.6.1", "1.6.2", "1.6.3", "1.6.4", "1.6.5", "1.6.6", "1.6.7", "1.6.8", "1.6.9", "1.6.10"]
# Init witness key: lists the public key followed by the corresponding private key
private-key = ["PPY6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]
```

Next, run the node with the prepared data directory: `/opt/peerplays/bin/witness_node -d testnet-data`. The node should start and begin producing blocks. It can be stopped by pressing `Ctrl-C`.

##### Building and Running the Contract Node
The official Peerplays node processes only the standard Peerplays blockchain functionality. As third party developers, we wish to write new smart contracts, and apps based upon them, which are not subject to the official blockchain consensus, but maintain their own consensus while leveraging the blockchain as a single source of truth regarding what operations have been processed, and in what order. To track our contract's operations and state according to our app's consensus, we need a new kind of _contract_ node to participate in the blockchain's P2P network, processing its consensus and ours alike.

Because the contract node is focused on third party contracts, it can be made leaner than an official node by removing code unrelated to the formal blockchain consensus. Furthermore, since third party contracts may be created or updated more often than those of the official blockchain, our contract node is also made more dynamic than the official node, allowing it to be configured and augmented at runtime. Admittedly, as of this writing, the contract node is somewhat rudimentary, but its dynamic design makes it an excellent foundation for future growth and expansion.

The contract node can be built, installed, and run with the following commands. The below CMake command sets the build type and the Peerplays path, which is where we installed Peerplays above. By default, the node will install alongside Peerplays.

```sh
$ git clone https://github.com/dapp-protocols/peerplays-contract-node # Clone the repo
$ mkdir peerplays-contract-node/build # Create build directory
$ cd peerplays-contract-node/build # Enter build directory
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DPEERPLAYS_PATH=/opt/peerplays .. # Configure
$ ninja # Build
$ ninja install # Install
$ /opt/peerplays/bin/ContractNode # Run the contract node
```

The contract node runs with no arguments. It starts up and creates a configuration directory (printed in its output) with a `genesis.json` file. The contract node does not have any seed nodes, but it listens for P2P connections on port 2776, so it can be connected to our testnet. To connect the contract node into the testnet, first overwrite the `genesis.json` file in its configuration directory with the one created for our testnet above, as they will not match by default, and they must match in order to make the nodes sync. Next, open the Peerplays node's `config.ini` file and add the contract node as a seed node like so:

```ini
# Set contract node as a seed node
seed-nodes = ["127.0.0.1:2776"]
```

> Note that when port 2776 is unavailable, the contract node chooses one at random. The port used is printed in the output logging.

IMPORTANT: When starting the Peerplays node, ensure that the chain ID logged at startup matches the one logged by the contract node, or else the nodes will not sync. If they do not match, it means that the two nodes are not configured with the same Genesis state. If this is the case, ensure the Peerplays node has a `genesis.json` file which is referenced in its `config.ini`, then destroy the testnet and begin afresh by stopping the node and re-running it with the `--resync-blockchain` flag. Next, delete all contents of the contract node's configuration directory, and copy the `genesis.json` file into this directory, then start the contract node. The nodes should now have the same Chain ID.

##### Building and Deploying a Contract for Peerplays
A contract can be built to deploy to our custom Peerplays node using the same process as with [EOSIO above](#building-a-contract-for-eosio), but by adding an additional argument to CMake: `-DPEERPLAYS_PATH=/opt/peerplays`. The CMake output should state that Peerplays was found and that the build is enabled. Continue building as usual: the same build directory should produce contract binaries for both EOSIO and Peerplays.

> NOTE: GCC version 9.3.0 is unable to build BAL contracts for Peerplays. Ubuntu 20.04 ships this version by default, but as of this writing, version 10.3.0 can also be installed simply by installing the `g++-10` package.
> 
> If building with GCC on Ubuntu 20.04, it is necessary to either set version 10.3.0 to be used system-wide using the `update-alternatives` command, or to direct CMake to use it with the `-DCMAKE_CXX_COMPILER=g++-10` flag to CMake.
> 
> Many thanks to Michel Santos for discovering this issue!

The Peerplays contract is built to a shared library in the `BAL/Peerplays` directory within the build directory, so for the supply chain contract, `BAL/Peerplays/libsupplychain.so`. To load the contract into the custom Peerplays node, add it to one of the plugin search paths which are printed in the log when the node starts up, such as `/opt/peerplays/lib/ContractNode/plugins`. The node should find the plugin in subsequent runs, or the plugin can be loaded dynamically into a running node by sending the node the `SIGUSR1` signal, i.e. by running `pkill -SIGUSR1 ContractNode`.

##### Running and Testing a Peerplays Contract
At this point, we should have a Peerplays node producing blocks on a testnet, and a contract node in sync with the Peerplays node, with our contract loaded into the contract node. Now, to run our contract, we execute a transaction on our testnet with a `custom_operation` that triggers our contract. To create transactions for the testnet, we use the Peerplays command line wallet program, `cli_wallet` in conjunction with our Peerplays node. To use the `cli_wallet`, we need the Peerplays node to open an RPC interface for it to connect to. We can set the RPC port in the config file:

```ini
# Open RPC socket for localhost (allows cli_wallet to connect)
rpc-endpoint = 127.0.0.1:8090
```

At this stage, the complete config file should look like this:

```
# Open RPC socket for localhost (allows cli_wallet to connect)
rpc-endpoint = 127.0.0.1:8090

# Set our contract node as the sole seed node
seed-nodes = ["127.0.0.1:2776"]

# Specify the Genesis file
genesis-json = <path to genesis.json>

# Init witness key: lists the public key followed by the corresponding private key
private-key = ["PPY6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]

# This tells the node to produce blocks even if no recent blocks are available
# This is disabled in production to prevent forking due to network failures, but it's necessary to start a new testnet
enable-stale-production = true

# Enable production with the genesis.json witness accounts
witness-id = "1.6.1"
witness-id = "1.6.2"
witness-id = "1.6.3"
witness-id = "1.6.4"
witness-id = "1.6.5"
witness-id = "1.6.6"
witness-id = "1.6.7"
witness-id = "1.6.8"
witness-id = "1.6.9"
witness-id = "1.6.10"
witness-id = "1.6.11"
```

After updating `config.ini`, restart the Peerplays node to reload its configuration. A log line at startup should confirm that it has opened a websocket RPC port. The `cli_wallet` looks for an RPC port on `localhost:8090` by default, so it should find the node without guidance; however, because we are using a testnet, it is necessary to give the `cli_wallet` the chain ID of our testnet on the command line. Note the chain ID from either node's startup logs, and run `cli_wallet --chain-id <chain ID>`. If using some other port than `8090`, specify the RPC address on the command line as in `-s ws://localhost:<port>`.

Once the wallet program is running, you should see a prompt: `>>> `. To verify that the wallet is properly connected, run `info` on its prompt, and it should print out information about the node's head block and blockchain status.

With the wallet now interfaced to the blockchain node, we want to create an account with which to invoke our contract. Take note that the contract is already loaded into the contract node, and unlike on EOSIO, this is all that is required to activate it. The contract is a third party construct and is ancillary to blockchain consensus, so it does not need to have an account to run it, nor is any transaction required to install it onto the chain: it's ready to go as soon as we are. To create an account with which to invoke the contract, we first need to initialize the wallet, load our testnet's `nathan` account into our wallet, and load the blockchain's tokens into the `nathan` account. We then use `nathan` to create a second testing account and send it tokens with which to pay fees on its transactions. This is all done with the commands below:

```
>>> set_password hi
>>> unlock hi
>>> import_key nathan 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
>>> import_balance nathan ["5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"] true
>>> upgrade_account nathan true
>>> create_account_with_brain_key testerbrainkey tester nathan nathan true
>>> transfer nathan tester 100000 PPY memo true
>>> get_account_id tester
```

For reference, the complete list of `cli_wallet` commands and their argument types (but not names) can be seen by running the `help` command. In practice, this isn't terribly useful, though, so it may be more enlightening to go straight to the [source](https://github.com/dapp-protocols/peerplays/blob/de87e1b82cfc147d7a4082d65f7a4c6046567d04/libraries/wallet/include/graphene/wallet/wallet.hpp#L304).

We now have created our `tester` account and can use it to invoke our contract. The contract is invoked using the [`custom_operation`](https://github.com/dapp-protocols/peerplays/blob/de87e1b82cfc147d7a4082d65f7a4c6046567d04/libraries/protocol/include/graphene/protocol/custom.hpp#L38) operation, where the contract action and arguments being invoked are specified in the `data` field according to a format recognized by the BAL.

The BAL supports two formats for contract invocation: a JSON-based string format, and a binary format. Any invocation may use either format at any time; however, it is recommended to use the binary format in production as it uses less space and, thus, a smaller fee. During development, however, the string format may be more convenient as it is human readable. We will be using the string format below.

The contents of the `data` field begin with a magic string that signifies to the BAL that the `custom_operation` is invoking a contract. This magic string is formatted as follows, where the contract name is as in the first argument to `BAL_CONTRACT` in the contract's main file, and the final `1.0s` indicates that the string format is selected. To select the binary format, `1.0b` is used instead.

`contract-action/<contract name>-1.0s` 

After the magic comes the action name and arguments. The string format for these is:

`<optional whitespace> <action name: text> <whitespace> <arguments JSON array>`

If there are no arguments being passed, the second whitespace and arguments array may be omitted.

Based on this format, then, if we wish to run the `add.wrhs` action with our `tester` account as the warehouse manager, we would format the `data` field as follows. Note that on Peerplays, accounts are referenced by ID rather than by name, so take the ID given when running `get_account_id tester` above, but drop the `1.2.` so only the last number is used. In my case, tester's ID was `1.2.20`, so I will use `20` below:

`contract-action/SupplyChain-1.0s add.wrhs [20, "Tester Warehouse"]`

Now that we know how to format our action invocation, we must use `cli_wallet` to put this into a transaction with a `custom_operation`. For this, we use the `run_custom_operation` command like so:

`>>> run_custom_operation tester ["tester"] "contract-action/SupplyChain-1.0s add.wrhs [20, \"Tester Warehouse\"]" 0 true`

In the above, the first argument indicates that `tester` will pay the fee. The second argument lists `tester` as an account authorizing the contract action. While the payer implicitly authorizes the transaction, in a BAL contract, it does not implicitly authorize the contract action: it must be listed in the authorizing accounts to affect the contract. The third argument is the data payload, which must escape the quotes (single quotes won't work). The fourth argument is the custom operation id, which can be zero as the BAL doesn't use it, and finally, `true` means to broadcast the transaction rather than just printing it.

Having run that command, we should see the contract node processing the action in its logs. The node will print the newly created table entry, so we can see that it is correct, but we can also dump the entire contract's database by sending the contract node the `SIGUSR2` signal, such as by running `pkill -SIGUSR2 ContractNode`:

```
Dumping database for contract: SupplyChain

Table 0:
object: {"id":"10.0.0","object":{"id":"TaggedID<warehouses>{0}","manager":20,"description":"Tester Warehouse"},"scope":"7235159537265672192"}
```

Now that we've created a warehouse, we can add inventory to it:

```
>>> run_custom_operation tester ["tester"] "contract-action/SupplyChain-1.0s add.invntry [0, 20, \"Reticulated Frobulator\", 1000]" 0 true
```

Note how we reference the warehouse as simply `0` -- it is also legal to provide the fully qualified ID, `"TaggedID<warehouses>{0}"`, but `0` is easier to type! Now we can dump the database again and see our inventory is stored:

```
Dumping database for contract: SupplyChain

Table 0:
object: {"id":"10.0.0","object":{"id":"TaggedID<warehouses>{0}","manager":20,"description":"Tester Warehouse"},"scope":"7235159537265672192"} 

Table 1:
object: {"id":"10.1.0","object":{"id":"TaggedID<inventory>{0}","description":"Reticulated Frobulator","origin":"a9995dafe63ad56e8e0915dd718763b75a6345e6","movement":[],"quantity":1000},"scope":0}
```

Next, we create a second account and try to use it to remove inventory from `tester`'s warehouse. We then attempt to do the same as `tester` himself.

```
>>> create_account_with_brain_key tester2brainkey tester2 nathan nathan true
>>> transfer nathan tester2 100000 PPY memo true
>>> run_custom_operation tester ["tester2"] "contract-action/SupplyChain-1.0s adj.invntry [0, 20, 0, null, [1, -10], \"Theft!\"]" 0 true
>>> run_custom_operation tester ["tester"] "contract-action/SupplyChain-1.0s adj.invntry [0, 20, 0, null, [1, -10], \"Sales\"]" 0 true
```

We see in the contract node logs that the first action is rejected. This also shows how, even though `tester` paid for the operation, because he is not listed in the authorizations, the action does not bear his authority.

```
[SupplyChain] Contract rejected operation with error: 10 assert_exception: Assert Exception
condition: Contract failed due to unsatisfied verification. Message: Required authorization of account 1.2.20 but no such authorization given
    {"MSG":"Required authorization of account 1.2.20 but no such authorization given"}
```

But the second action, being properly authenticated, succeeds. We can dump the database to see that the quantity in stock has been adjusted:

```
Dumping database for contract: SupplyChain

Table 0:
object: {"id":"10.0.0","object":{"id":"TaggedID<warehouses>{0}","manager":20,"description":"Tester Warehouse"},"scope":"7235159537265672192"} 

Table 1:
object: {"id":"10.1.0","object":{"id":"TaggedID<inventory>{0}","description":"Reticulated Frobulator","origin":"a9995dafe63ad56e8e0915dd718763b75a6345e6","movement":[],"quantity":990},"scope":0}
```

We have successfully run our contract on Peerplays and tested it to ensure that the actions are properly authenticated and the database is updated correctly.

### Where do We Go from Here?
Congratulations on getting your BAL-based smart contract development environment up and running! Furthermore, thank you for joining me on this journey. It is my belief that this system for writing smart contracts is more approachable and straightforward than has been demonstrated elsewhere in the industry. I hope that you agree, and having completed this tutorial, are already beginning to imagine the possibilities for your contracts. For further guidance on how to design and build novel smart contracts on the BAL, check out the [contract tutorial](Tutorial.md) which covers how the Supply Chain smart contract we used above came to be.

Of course, a smart contract is only the first step toward implementing a full decentralized application. Smart contracts, like the blockchains they run on, are server-side solutions, and do not operate on end user computers. Applications and GUIs, however, exist entirely on end user hardware. Bridging this gap is the purpose of The DApp Protocols and the Hyperverse project, of which the BAL, providing a stable and portable platform for back-ends, is the first part.

The greater vision of a full, end-to-end solution that reaches users with intuitive and trustworthy decentralized applications is still only beginning to germinate, and I welcome help bringing all parts of this vision to fruition. While the BAL continues to be an active site of development with much work yet to be completed, efforts are also in progress to extend the reach of decentralized software toward the front-end in the form of the [Pollaris DApp](https://github.com/FollowMyVote/pollaris-gui), both a contract back-end and GUI front-end. Other related efforts are infrastructures to support the rapid development of tooling and back-end/middleware technology, in the form of [Infra](https://github.com/dapp-protocols/Infra); and to facilitate the creation of highly intuitive graphical applications which dynamically self-construct to adapt to users' individual environments and needs, in the form of [Qappa](https://github.com/dapp-protocol/Qappa). 

Again, while this vision is broad in reach and is still in its early phases of realization, I hope that what I have shown so far exemplifies the pragmatic feasibility of the vision and the practical usability of the development platforms I am building. Not only is it possible, it has become inevitable. Nevertheless, it's going to take a lot of work to bring it to the market, and I don't plan to do it alone. I look forward to seeing you in the repos!
