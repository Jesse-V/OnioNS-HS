cmake_minimum_required(VERSION 2.8)

project(onions-hs)

# define compiler options for Clang and GCC/G++
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fstack-protector -D_FORTIFY_SOURCE=1")
SET(CLANG_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -O2 -Wno-c++98-compat-pedantic -pedantic -Weverything -Wno-exit-time-destructors -Wno-weak-vtables -Wno-padded -Wno-deprecated -Wno-documentation -Wno-documentation-unknown-command -Wno-reserved-id-macro -Wno-missing-noreturn -Wno-sign-conversion -Wno-shadow")
SET(GCC_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -O2 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wunsafe-loop-optimizations")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CLANG_DEBUG}")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_DEBUG "${GCC_DEBUG}")
endif()

include_directories(${CMAKE_INSTALL_PREFIX}/include/onions-common /usr/include/botan-1.10)
add_definitions(-DINSTALL_PREFIX=std::string\("${CMAKE_INSTALL_PREFIX}"\))

add_executable(onions-hs
  main.cpp
  RecordManager.cpp
  Terminal.cpp
)

FIND_LIBRARY(botan NAMES "botan-1.10" REQUIRED)
FIND_LIBRARY(argtable2 NAMES "argtable2" REQUIRED)
FIND_LIBRARY(jsoncpp NAMES "jsoncpp" REQUIRED)

# find main onions-common library components
FIND_LIBRARY(onions-common NAMES "onions-common" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)

# find json-rpc-cpp components
FIND_LIBRARY(onions-jsonrpccpp-common NAMES "jsonrpccpp-common" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)
FIND_LIBRARY(onions-jsonrpccpp-client NAMES "jsonrpccpp-client" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)

if (${botan} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "Botan could not be found! Please install libbotan1.10-dev")
else()
  message(STATUS "Found Botan library at ${botan}")
endif()

if (${argtable2} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "Argtable2 could not be found! Please install libargtable2-dev")
else()
  message(STATUS "Found Argtable2 library at ${argtable2}")
endif()

if (${jsoncpp} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "Could not find jsoncpp. Please install libjsoncpp-dev")
else()
  message(STATUS "Found jsoncpp at ${jsoncpp}")
endif()

if (${onions-common} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "OnioNS-common could not be found! Please install tor-onions-common")
else()
  message(STATUS "Found OnioNS-common library at ${onions-common}")
endif()

if (${onions-jsonrpccpp-client} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "JSON-RPC-CPP component was not found! Please install tor-onions-common")
else()
  message(STATUS "Found JSON-RPC-CPP's Client at ${onions-jsonrpccpp-client}")
endif()

target_link_libraries(onions-hs ${onions-common} ${jsoncpp}
  ${onions-jsonrpccpp-common} ${onions-jsonrpccpp-client} ${argtable2} ${botan} pthread)

#installation of files
set(ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
install(TARGETS onions-hs                       DESTINATION bin)
install(FILES ${ASSETS}/onions-hs.1.gz          DESTINATION share/man/man1)
install(FILES ${ASSETS}/american-english-small  DESTINATION share/onions-hs)
