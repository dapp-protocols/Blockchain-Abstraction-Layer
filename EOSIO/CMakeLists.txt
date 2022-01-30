add_contract("${CONTRACT_NAME}" "${CONTRACT_NAME}" ${CONTRACT_SOURCES})
set_target_properties("${CONTRACT_NAME}.wasm" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CONTRACT_NAME}")
target_compile_definitions("${CONTRACT_NAME}.wasm" PUBLIC BAL_PLATFORM_EOSIO)
target_include_directories("${CONTRACT_NAME}.wasm" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Include")

# These don't help the build at all, but they do help IDEs
target_include_directories("${CONTRACT_NAME}.wasm" PRIVATE
                           "${EOSIO_CDT_ROOT}/eosio.cdt/include/eosiolib/contracts"
                           "${EOSIO_CDT_ROOT}/eosio.cdt/include/eosiolib/core"
                           "${EOSIO_CDT_ROOT}/eosio.cdt/include")
