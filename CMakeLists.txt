cmake_minimum_required(VERSION 3.10)

function(BAL_CONTRACT)
    cmake_parse_arguments(PARSE_ARGV 0 CONTRACT "" "NAME;BAL_PATH" "SOURCES")
    message(STATUS "Defining BAL contract targets for contract ${CONTRACT_NAME}")

    # Get absolute source paths
    foreach(SOURCE ${CONTRACT_SOURCES})
        get_filename_component(ABSOLUTE_SOURCE "${SOURCE}" ABSOLUTE)
        list(APPEND ABSOLUTE_CONTRACT_SOURCES "${ABSOLUTE_SOURCE}")
    endforeach()
    set(CONTRACT_SOURCES ${ABSOLUTE_CONTRACT_SOURCES})

    # Add BAL to include paths
    include_directories("${CONTRACT_BAL_PATH}")

    # Evaluate Peerplays contract build target
    set(PEERPLAYS_PATH "/opt/peerplays" CACHE STRING "Path to the Peerplays installation")
    if (EXISTS "${PEERPLAYS_PATH}/lib/libgraphene_chain.so")
        set(PEERPLAYS_FOUND true)
    endif()

    # If Peerplays is found, enable build for Peerplays contract
    if (PEERPLAYS_FOUND)
        message("Peerplays found; enabling Peerplays contract build")

        add_subdirectory("${CONTRACT_BAL_PATH}/Peerplays" "${CMAKE_CURRENT_BINARY_DIR}/BAL/Peerplays")
    else()
        message("Peerplays not found; disabling Peerplays contract build")
    endif()

    # Evaluate EOSIO contract build target
    find_package(eosio.cdt HINTS "${CDT_PATH}")
    if (eosio.cdt_FOUND)
        message("EOSIO CDT Found; enabling EOSIO contract build")

        add_subdirectory("${CONTRACT_BAL_PATH}/EOSIO" "${CMAKE_CURRENT_BINARY_DIR}/BAL/EOSIO")
    else()
        message("EOSIO CDT Not found; disabling EOSIO contract build")
    endif()
endfunction()
