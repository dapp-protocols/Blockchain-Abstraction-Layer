project(SupplyChainExample)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

# Include the BAL CMake code
include(../CMakeLists.txt)

# Declare the contract
BAL_CONTRACT(NAME supplychain BAL_PATH .. SOURCES SupplyChain.cpp main.cpp)
