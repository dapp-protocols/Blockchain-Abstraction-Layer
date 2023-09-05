# How to Set Up a BAL Contract Development Environment

In this document, we will explore how to set up a development environment for building BAL contracts on the developer's host OS. This is in contrast to other documentation covering how to set up a containerized development environment which uses Docker containers to ship the toolchains and nodes used in the BAL contract development process: these instructions cover how to build the necessary componentry from scratch and install them into your local operating system.

This tutorial is written for developers experienced with Linux operating systems, who are accustomed to compiling software from source and have at least basic command line proficiency. It makes minimal assumptions about the reader's environment and aims to provide generally applicable instructions which can be readily interpreted for any Unix-like environment.

### Leap Development Environment
The Leap contract development environment is compiled in three steps. The first step is to build LLVM with the necessary flags for Leap. The second step is to build the Leap node, which will be used to launch local test blockchain networks to deploy and test the prototype contract on. The third step is to build and install the Leap Contract Development Toolkit (CDT), which is the toolchain and standard libraries for Leap contract development. Once the CDT is installed, we can compile a contract and load it into our testnet for testing.

##### LLVM/Clang
LVM/Clang can be built according to the official [instructions](https://clang.llvm.org/get_started.html). Note that, as Leap does not support the latest versions of LLVM, it is necessary to use an older version. At the time of this writing, version 11.1 is the latest supported version. This can be selected in the initial clone command by passing `--branch llvmorg-11.1.0`.

When building, it is necessary to add the following flags when running CMake to build it for Leap:

```cmake
-DLLVM_ENABLE_RTTI=1 -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
'-DLLVM_ENABLE_PROJECTS=clang;compiler-rt' -DCMAKE_INSTALL_PREFIX=/opt/clang-leap
```

While it is optional, I like to add the `-DCMAKE_INSTALL_PREFIX=/opt/clang-leap` flag so that this copy of LLVM is set aside as a special build for Leap and does not interfere with my main (packaged) LLVM.

> **Troubleshooting:**
> When building LLVM 11.1.0 in an updated environment, some build errors may occur due to the environment being more recent than the target LLVM. These compatibility issues have been fixed in updated versions of LLVM, and can be backported to LLVM 11.1.0 by cherry picking commits. (Cherry picking won't work if the repo was cloned with `--depth=1`, so in this case, just follow the Ref links and apply the changes manually).
>  - `llvm-project/compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.cpp:133:10: fatal error: linux/cyclades.h: No such file or directory`
>    - Run `git cherry-pick 884040db086936107ec81656aa5b4c607235fb9a`
>    - Ref https://github.com/llvm/llvm-project/commit/884040db086936107ec81656aa5b4c607235fb9a
>  - `llvm-project/llvm/utils/benchmark/src/benchmark_register.h:17:30: error: ‘numeric_limits’ is not a member of ‘std’`
>    - Run `git cherry-pick b498303066a63a203d24f739b2d2e0e56dca70d1`
>    - Ref https://github.com/llvm/llvm-project/commit/b498303066a63a203d24f739b2d2e0e56dca70d1

##### Building Leap Node
Clone the Leap node source code repository from `https://github.com/AntelopeIO/leap`, create a `build` directory within the repository, and `cd` into it. Run CMake to configure the build. Here's an example of some commands to do this:

```sh
$ git clone https://github.com/AntelopeIO/leap --recursive
$ mkdir leap/build
$ cd leap/build
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/opt/clang-leap/lib/cmake/llvm \
  -DCMAKE_INSTALL_PREFIX=/opt/leap -DCMAKE_INSTALL_SYSCONFDIR=/opt/leap/etc \
  -DCMAKE_INSTALL_LOCALSTATEDIR=/opt/leap/var ..
```

Note the `--recursive` flag on the clone. This is important, as it causes git to clone all of the submodules during the initial clone. If you forget it, you'll need to get them all after the fact by running `git submodule update --init --recursive`.

Also note the many flags to CMake. The `-G Ninja` causes CMake to write Ninja build files instead of Makefiles, as I prefer Ninja. Readers who prefer to use make should omit this flag. The build type can be adjusted to the reader's preference. The `LLVM_DIR` flag indicates to CMake where our custom built clang is. The `CMAKE_INSTALL_*` flags are used to cause CMake to install the project to `/opt/leap` rather than `/usr/local`, which I find preferable as it keeps my main OS directories clear of files not managed by my package manager. Readers should feel free to tune these variables to their preference.

> **Troubleshooting:**
> - In environments which have only dynamic Boost libraries, CMake will fail to find Boost libraries saying "no suitable build variant has been found". Work around this issue by adding the CMake argument `-DBoost_USE_STATIC_LIBS=OFF`
> - In some environments, building fails with errors circa a `hash<>` template referenced from FC. Work around this issue by adding a `#include <boost/container_hash/hash.hpp>` to `libraries/fc/include/fc/crypto/sha256.hpp`
> - In some environments, building fails due to missing `std::list` in FC. Work around this issue by adding a `#include <list>` to `libraries/fc/include/fc/io/raw.hpp`
> - `leap/libraries/eos-vm/include/eosio/vm/execution_context.hpp:273:61: error: no matching function for call to ‘max(int, long int)’`
>   - Manually apply [this commit](https://github.com/EOSIO/eos-vm/pull/228/commits/6fd38668d6fa1a909fb1322b676ba43ff56f14e2) to eos-vm in `libraries/eos-vm` directory
>   - Ref https://github.com/EOSIO/eos-vm/issues/227

After configuring, build and install the node, for example with `ninja && ninja install`. Note that, by default, a barebones install is performed which includes only the binaries, licenses, and some helper scripts. To perform a complete development install including libraries and headers, install the `dev` component by running `cmake -DCOMPONENT=dev -P cmake_install.cmake`.

##### Launching a Leap Testnet
Now that the Leap node is installed, we launch a private testnet so we can produce blocks and test our contract locally. The command is as follows:

```sh
$ export PATH="$PATH:/opt/leap/bin"
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

##### Building Leap CDT
Similarly to the node, we clone the repo, make a build directory, and configure. Note that, pending inclusion of some fixes upstream, it is necessary to use the DApp Protocols fork to get full support.

```sh
$ git clone https://github.com/dapp-protocols/leap-cdt --recursive
$ mkdir leap-cdt/build
$ cd leap-cdt/build
$ export CMAKE_PREFIX_PATH="/opt/leap:$CMAKE_PREFIX_PATH"
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/leap-cdt ..
```

The options here are similar to the options in the node above, and follow the same rules; however, it is necessary to add the `CMAKE_PREFIX_PATH` as an environment variable as the CDT's cmake configuration may not properly propagate this value if passed to CMake directly.

> **Troubleshooting:**
> - If it was necessary to specify `-DBoost_USE_STATIC_LIBS=OFF` when building Leap above, it will be necessary here as well. Moreover, when building fails even with that flag set, edit `build/tests/integration/CMakeCache.txt` and set `Boost_USE_STATIC_LIBS=ON` to `OFF` as well, then try building again.
> - The CDT builds a custom LLVM for building the contracts, and as with the LLVM build above, this LLVM may also fail to build with `error: ‘numeric_limits’ is not a member of ‘std’`.
>   - Cherry picking doesn't work here, so manually edit `eosio_llvm/utils/benchmark/src/benchmark_register.h`
>   - Add `#include <limits>` near beginning of file
> - include/eosiolib/contracts/eosio/multi_index.hpp:443:102: error: function 'primary_key' with deduced return type cannot be used before it is defined
>   - Honestly, I don't know what is going wrong here. Maybe a compiler bug due to the old (clang 9.0.1, circa 2019) compiler being used? The code looks valid to me.
>   - Fix is to edit `tests/unit/test_contracts/explicit_nested_tests.cpp`
>     - Find line `auto primary_key() const { return id; }`
>     - Replace with `uint64_t primary_key() const { return id; }`

Now, build and install the CDT, i.e. `ninja && ninja install`. Note that this build takes quite a while and is usually working even when it doesn't appear as such.

##### Building a Contract for Leap
Now that the Leap build environment is established, we can use it to build a contract. Let's build the BAL's example supply chain contract.

First, check out the BAL repo, make a `build` directory in the `Example` subdirectory, then use CMake to configure the build, and finally, build the contract.

```sh
$ git clone https://github.com/dapp-protocols/Blockchain-Abstraction-Layer
$ mkdir bal/Example/build
$ cd bal/Example/build
$ cmake -G Ninja -DCDT_PATH=/opt/leap-cdt ..
$ ninja
$ ls BAL/Leap/supplychain
```

The contract is built into the `BAL/Leap` directory. Unfortunately, the abigen doesn't handle our tagged IDs quite as nicely as we would like, so it is ideal to edit the auto generated ABI in `BAL/Leap/supplychain/Leap-supplychain.abi` to change the types of the tagged IDs from their struct types to plain `uint64`s. For example, change `CargoId`'s definition from this:

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

This should be done for `CargoId`, `InventoryId`, `ManifestId`, and `WarehouseId` (but not `TransactionId`!). After this, it is ideal to remove the four `ID_NameTag_#####` entries from the `structs` section entirely. While these edits to the ABI are not necessary, if they are omitted, IDs in the contract must be written in JSON as, for example, `{"value": 7}` rather than simply `7`.

##### Deploying and Running an Leap Contract
To deploy and run a contract on Leap, first ensure that `nodeos` is up and producing blocks as described [above](#launching-a-leap-testnet). With the testnet operational, run the following commands to prepare the contract account, load the contract, and test it:

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
$ cleos set contract supplychain <path to contract build>/BAL/Leap/supplychain
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
$ # Note that if the last command failed with a `Pack data exception`, it likely means the ABI wasn't edited as described above, and the `warehouseId` must be written as `{"value": 0}`
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

### Where do We Go from Here?
Congratulations on getting your BAL-based smart contract development environment up and running! Furthermore, thank you for joining me on this journey. It is my belief that this system for writing smart contracts is more approachable and straightforward than has been demonstrated elsewhere in the industry. I hope that you agree, and having completed this tutorial, are already beginning to imagine the possibilities for your contracts. For further guidance on how to design and build novel smart contracts on the BAL, check out the [contract tutorial](Tutorial.md) which covers how the Supply Chain smart contract we used above came to be.

Of course, a smart contract is only the first step toward implementing a full decentralized application. Smart contracts, like the blockchains they run on, are server-side solutions, and do not operate on end user computers. Applications and GUIs, however, exist entirely on end user hardware. Bridging this gap is the purpose of The DApp Protocols and the Hyperverse project, of which the BAL, providing a stable and portable platform for back-ends, is the first part.

The greater vision of a full, end-to-end solution that reaches users with intuitive and trustworthy decentralized applications is still only beginning to germinate, and I welcome help bringing all parts of this vision to fruition. While the BAL continues to be an active site of development with much work yet to be completed, efforts are also in progress to extend the reach of decentralized software toward the front-end in the form of the [Pollaris DApp](https://github.com/FollowMyVote/pollaris-gui), both a contract back-end and GUI front-end. Other related efforts are infrastructures to support the rapid development of tooling and back-end/middleware technology, in the form of [Infra](https://github.com/dapp-protocols/Infra); and to facilitate the creation of highly intuitive graphical applications which dynamically self-construct to adapt to users' individual environments and needs, in the form of [Qappa](https://github.com/dapp-protocols/Qappa).

Again, while this vision is broad in reach and is still in its early phases of realization, I hope that what I have shown so far exemplifies the pragmatic feasibility of the vision and the practical usability of the development platforms I am building. Not only is it possible, it has become inevitable. Nevertheless, it's going to take a lot of work to bring it to the market, and I don't plan to do it alone. I look forward to seeing you in the repos!

