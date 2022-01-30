find_package(Threads)
find_package(Boost REQUIRED system filesystem serialization chrono date_time coroutine context thread)

find_library(FC_LIB REQUIRED NAMES fc fc_debug HINTS "${PEERPLAYS_PATH}/lib")

set(PEERPLAYS_LIBS graphene_chain graphene_net graphene_db graphene_protocol graphene_utilities
    ${FC_LIB} secp256k1 z Threads::Threads ssl crypto curl ${CMAKE_DL_LIBS})

add_library("${CONTRACT_NAME}" MODULE ${CONTRACT_SOURCES})
target_compile_definitions("${CONTRACT_NAME}" PUBLIC BAL_PLATFORM_PEERPLAYS)
target_include_directories("${CONTRACT_NAME}" PRIVATE "${PEERPLAYS_PATH}/include" "${CMAKE_CURRENT_BINARY_DIR}/Include")
target_link_directories("${CONTRACT_NAME}" PRIVATE
                        "${PEERPLAYS_PATH}/lib" "${PEERPLAYS_PATH}/lib/cryptonomex")
target_link_libraries("${CONTRACT_NAME}" ${PEERPLAYS_LIBS} ${Boost_LIBRARIES})
