include("${CDT_ROOT}/lib/cmake/cdt/CDTWasmToolchain.cmake")

add_contract("${CONTRACT_NAME}" "${CONTRACT_NAME}" ${CONTRACT_SOURCES})
set_target_properties("${CONTRACT_NAME}" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CONTRACT_NAME}")
target_compile_definitions("${CONTRACT_NAME}" PUBLIC BAL_PLATFORM_LEAP)
target_include_directories("${CONTRACT_NAME}" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Include")
target_compile_options("${CONTRACT_NAME}" PRIVATE -Wno-attributes)

# These don't help the build at all, but they do help IDEs
target_include_directories("${CONTRACT_NAME}" PRIVATE
                           "${CDT_ROOT}/cdt/include/eosiolib/contracts"
                           "${CDT_ROOT}/cdt/include/eosiolib/core"
                           "${CDT_ROOT}/cdt/include")
