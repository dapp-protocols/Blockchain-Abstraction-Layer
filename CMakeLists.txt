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

    # Evaluate Leap contract build target
    find_package(cdt HINTS "${CDT_PATH}")
    if (cdt_FOUND)
        message("Leap CDT Found; enabling Leap contract build")

        add_subdirectory("${CONTRACT_BAL_PATH}/Leap" "${CMAKE_CURRENT_BINARY_DIR}/BAL/Leap")
    else()
        message("Leap CDT Not found; disabling Leap contract build")
    endif()
endfunction()
