cmake_minimum_required(VERSION 3.19)

project("HPX Optimization cases" CXX)

option(WITH_CUSTOM_HPX "Use available HPX" OFF)
option(WITH_CUSTOM_GOOGLE_BENCH "Use available googlebench" OFF)

include(FetchContent)

if(WITH_CUSTOM_GOOGLE_BENCH)
  find_package(googlebench REQUIRED)
else()
  set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")

  FetchContent_Declare(
      googlebenchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.8.3
  )
  
  FetchContent_MakeAvailable(googlebenchmark)
endif()

if(WITH_CUSTOM_HPX)
  find_package(HPX REQUIRED)
else()
  # Installing Boost
  FetchContent_Declare(Boost
      URL https://github.com/boostorg/boost/releases/download/boost-1.81.0/boost-1.81.0.tar.gz
      TLS_VERIFY true
  )
  FetchContent_Populate(Boost)

  set(EX_PRC "Execute process" CACHE STRING "Used by command line tool.")
  set(EX_PRC_INTERNAL "" CACHE INTERNAL "for internal use only; do not modify")

  if(NOT EX_PRC STREQUAL EX_PRC_INTERNAL)
    if(WIN32)
      execute_process(COMMAND cmd /C "cd ${CMAKE_BINARY_DIR}\\_deps\\boost-src && .\\bootstrap.bat && .\\b2 headers cxxflags=/std:c++20" )
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      execute_process(COMMAND sh -c "cd ${CMAKE_BINARY_DIR}/_deps/boost-src && ./bootstrap.sh --prefix=${CMAKE_BINARY_DIR}/_deps/boost-installed && ./b2 install --prefix=${CMAKE_BINARY_DIR}/_deps/boost-installed cxxflags=--std=c++20" )
    else()
      execute_process(COMMAND sh -c "cd ${CMAKE_BINARY_DIR}/_deps/boost-src && ./bootstrap.sh && ./b2 headers cxxflags=--std=c++20" )
    endif()
    set(EX_PRC_INTERNAL ${EX_PRC} CACHE INTERNAL "for internal use only; do not modify")
  endif()
  
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(Boost_DIR "${CMAKE_BINARY_DIR}/_deps/boost-installed/lib/cmake")
    set(Boost_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/boost-installed/include")
  else()
    set(Boost_DIR "${CMAKE_BINARY_DIR}/_deps/boost-src")
    set(Boost_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/boost-src")
  endif()
  
  # Installing HWloc  
  
  if(UNIX)
    FetchContent_Declare(HWLoc
      URL https://download.open-mpi.org/release/hwloc/v2.9/hwloc-2.9.3.tar.gz
      TLS_VERIFY true
    )
    if(NOT HWLoc_POPULATED)
      FetchContent_Populate(HWLoc)
      execute_process(COMMAND sh -c "cd ${CMAKE_BINARY_DIR}/_deps/hwloc-src && ./configure --prefix=${CMAKE_BINARY_DIR}/_deps/hwloc-installed && make -j && make install")
    endif()
    set(HWLOC_ROOT "${CMAKE_BINARY_DIR}/_deps/hwloc-installed")
  elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win64")
    FetchContent_Declare(HWLoc
      URL https://download.open-mpi.org/release/hwloc/v2.9/hwloc-win64-build-2.9.3.zip
      TLS_VERIFY true
    )
    if(NOT HWLoc_POPULATED)
      FetchContent_Populate(HWLoc)
    endif()
    set(HWLOC_ROOT "${CMAKE_BINARY_DIR}/_deps/hwloc-src" CACHE INTERNAL "")
    find_package(hwloc REQUIRED PATHS ${HWLOC_ROOT} NO_DEFAULT_PATH)
    include_directories(${HWLOC_ROOT}/include)
    link_directories(${HWLOC_ROOT}/lib)
    set(Hwloc_INCLUDE_DIR ${HWLOC_ROOT}/include CACHE INTERNAL "")
    set(Hwloc_LIBRARY ${HWLOC_ROOT}/lib CACHE INTERNAL "")
  else()
    FetchContent_Declare(HWLoc
      URL https://download.open-mpi.org/release/hwloc/v2.9/hwloc-win64-build-2.9.3.zip
      TLS_VERIFY true
    )
    if(NOT HWLoc_POPULATED)
      FetchContent_Populate(HWLoc)
    endif()
    set(HWLOC_ROOT "${CMAKE_BINARY_DIR}/_deps/hwloc-src" CACHE INTERNAL "")
    include_directories(${HWLOC_ROOT}/include)
    link_directories(${HWLOC_ROOT}/lib)
    set(Hwloc_INCLUDE_DIR ${HWLOC_ROOT}/include CACHE INTERNAL "")
    set(Hwloc_LIBRARY ${HWLOC_ROOT}/lib CACHE INTERNAL "")
  endif() # End hwloc installation
  

  # Installing HPX
  FetchContent_Declare(HPX
    URL https://github.com/STEllAR-GROUP/hpx/archive/refs/tags/v1.9.1.tar.gz
    TLS_VERIFY true
  )
  FetchContent_GetProperties(HPX)

  if(NOT HPX_POPULATED)
    set(HPX_WITH_CXX_STANDARD 20  CACHE INTERNAL "")
    set(HPX_WITH_FETCH_ASIO ON  CACHE INTERNAL "")
    set(HPX_WITH_MALLOC "system"  CACHE INTERNAL "")
    set(HPX_WITH_EXAMPLES OFF  CACHE INTERNAL "")
    set(HPX_WITH_TESTS OFF  CACHE INTERNAL "")
    FetchContent_Populate(HPX)
    add_subdirectory(${hpx_SOURCE_DIR} ${hpx_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

add_executable(opt_bench src/main.cpp)

# Link Shlwapi to the project
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
  set(_EXTRA_LIBS_ Shlwapi)
else()
  set(_EXTRA_LIBS_ "")
endif()

target_link_libraries(opt_bench
 PRIVATE HPX::hpx 
 PRIVATE HPX::wrap_main
 PRIVATE HPX::iostreams_component
 PRIVATE benchmark::benchmark
 PRIVATE ${_EXTRA_LIBS_}
)


if(WIN32 OR "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win64" AND NOT WITH_CUSTOM_HPX)
  add_custom_command(TARGET opt_bench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${hpx_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
        $<TARGET_FILE_DIR:opt_bench>
  )
  add_custom_command(TARGET opt_bench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${HWLOC_ROOT}/bin/libhwloc-15.dll"
        $<TARGET_FILE_DIR:opt_bench>
  )
endif()

